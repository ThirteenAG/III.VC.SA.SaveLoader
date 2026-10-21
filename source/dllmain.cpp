#include "stdafx.h"

// The plugin's settings: described in settings/settings.json (embedded as a resource), stored in the .cfg next to the
// plugin
inline asi_settings::Plugin& Settings()
{
    static asi_settings::Plugin plugin;
    static bool loaded = plugin.Load();
    (void)loaded;
    return plugin;
}

ASI_SETTINGS_EXPORT(Settings())

namespace
{
    enum class Game { III, VC, SA };

    // MAIN.LoadSlot: -1 off, 0 the most recent save, 1-8 a slot, 9 a new game
    constexpr int LoadOff = -1;
    constexpr int LoadMostRecent = 0;
    constexpr int LoadNewGame = 9;

    Game game;
    int loadSlot = LoadOff;
    bool skipLoad = false;          // Shift was held when the game started
    char* saveNamePrefix = nullptr; // the game's save path without the slot number: "...\GTA3sf"
    char userFilesFolder[MAX_PATH] = {};

    template<class T> T& At(uintptr_t base, ptrdiff_t offset)
    {
        return *reinterpret_cast<T*>(base + offset);
    }

    char* __cdecl UserFilesFolder()
    {
        CreateDirectoryA(userFilesFolder, nullptr);
        return userFilesFolder;
    }

    // The slot (1-8) of the most recent save, 0 when there is none. III and VC store the time of saving in the save,
    // otherwise the file's time counts.
    int FindMostRecentSave()
    {
        if (!saveNamePrefix || !*saveNamePrefix)
            return 0;
        std::filesystem::path prefix(saveNamePrefix);
        auto folder = prefix.parent_path();
        int best = 0;
        FILETIME bestTime{};
        WIN32_FIND_DATAA data;
        HANDLE find = FindFirstFileA((std::string(saveNamePrefix) + "*.b").c_str(), &data);
        if (find == INVALID_HANDLE_VALUE)
            return 0;
        do
        {
            // "GTA3sf3.b": the digits before the extension
            std::string name = data.cFileName;
            auto end = name.size() - 2;
            auto start = end;
            while (start > 0 && isdigit(static_cast<unsigned char>(name[start - 1])))
                --start;
            if (start == end || end - start > 2)
                continue;
            int slot = std::atoi(name.substr(start, end - start).c_str());
            if (slot < 1 || slot > 8)
                continue;

            FILETIME time = data.ftLastWriteTime;
            if (game != Game::SA)
            {
                if (FILE* file = _wfopen((folder / data.cFileName).c_str(), L"rb"))
                {
                    SYSTEMTIME saved{};
                    FILETIME converted{};
                    if (fseek(file, 0x34, SEEK_SET) == 0 && fread(&saved, sizeof(saved), 1, file) == 1 &&
                        SystemTimeToFileTime(&saved, &converted))
                        time = converted;
                    fclose(file);
                }
            }
            if (!best || CompareFileTime(&time, &bestTime) > 0)
            {
                best = slot;
                bestTime = time;
            }
        } while (FindNextFileA(find, &data));
        FindClose(find);
        return best;
    }

    // The slot to load now (1-8) or LoadNewGame
    int ChooseSlot(bool(__cdecl* checkSlotDataValid)(int))
    {
        int slot = loadSlot == LoadMostRecent ? FindMostRecentSave() : loadSlot;
        if (slot >= 1 && slot <= 8 && checkSlotDataValid(slot - 1))
            return slot;
        return LoadNewGame;
    }

    // ---------------------------------------------------------------------------------------------------------------
    // GTA III and Vice City

    namespace RW3
    {
        uintptr_t frontendIdleCall = 0;
        void(__cdecl* frontendIdle)() = nullptr;
        bool(__cdecl* checkSlotDataValid)(int) = nullptr;
        uintptr_t menuManager = 0;
        void(__fastcall* processOnOffMenuOptions)(uintptr_t) = nullptr;   // VC
        ProtectedGameRef<uint8_t> newGameMenuFlag;                          // III: the menu field a new game clears
        SafetyHookMid newGameHook;                                           // III

        void __cdecl FrontendIdle()
        {
            // Once: the original call again
            injector::MakeCALL(frontendIdleCall, frontendIdle, true);

            if (!skipLoad)
            {
                bool iii = game == Game::III;
                int slot = ChooseSlot(checkSlotDataValid);
                if (processOnOffMenuOptions)
                    processOnOffMenuOptions(menuManager);

                if (slot == LoadNewGame)
                {
                    if (iii && newGameMenuFlag.is_initialized())
                    {
                        newGameMenuFlag = 0x10;
                        (void)newGameHook.enable();
                    }
                    At<uint32_t>(menuManager, iii ? 0x548 : 0xF8) = iii ? 10 : 7;
                }
                else
                {
                    At<uint8_t>(menuManager, iii ? 0x55C : 0x100) = uint8_t(slot - 1);   // the selected slot
                    At<uint8_t>(menuManager, iii ? 0x548 : 0xF8) = iii ? 14 : 12;        // the load item
                }
                At<uint8_t>(menuManager, iii ? 0x115 : 0x11) = 1;
                At<uint8_t>(menuManager, iii ? 0x111 : 0x38) = 0;
                At<uint8_t>(menuManager, iii ? 0x114 : 0x39) = 1;
                At<uint8_t>(menuManager, iii ? 0x454 : 0x3C) = 1;
            }
            frontendIdle();
        }

        void Init()
        {
            bool iii = game == Game::III;
            auto pattern = hook::pattern("68 ? ? ? ? 68 ? ? ? ? 50 C7 84 24 80 00 00 00 00");
            if (!pattern.empty())
                saveNamePrefix = *pattern.get_first<char*>(1);

            if (*userFilesFolder)
            {
                auto a = hook::pattern("E8 ? ? ? ? 50 E8 ? ? ? ? 59 C3");
                if (iii)
                {
                    auto b = hook::pattern("E8 ? ? ? ? 50 E8 ? ? ? ? 59 E8");
                    auto c = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 C5 59 85 ED");
                    if (!a.empty() && !b.empty() && c.size() > 1)
                    {
                        injector::MakeCALL(a.get_first(), UserFilesFolder, true);
                        injector::MakeCALL(b.get_first(), UserFilesFolder, true);
                        injector::MakeCALL(c.get(1).get<void>(), UserFilesFolder, true);
                    }
                }
                else
                {
                    // VC also has the folder's code inlined in its startup: call ours there and skip it
                    auto b = hook::pattern("E8 ? ? ? ? 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 89 C5");
                    auto inlined = hook::pattern("C7 05 ? ? ? ? 00 00 00 00 C7 05 ? ? ? ? 00 00 00 00 E8");
                    auto resume = hook::pattern("50 E8 ? ? ? ? 59 E8 ? ? ? ? C7 05 ? ? ? ? 00 00 00 00");
                    if (!a.empty() && b.size() > 2 && inlined.size() > 17 && !resume.empty())
                    {
                        injector::MakeCALL(a.get_first(), UserFilesFolder, true);
                        injector::MakeCALL(b.get(2).get<void>(), UserFilesFolder, true);
                        injector::MakeCALL(inlined.get(17).get<void>(25), UserFilesFolder, true);
                        injector::MakeJMP(inlined.get(17).get<void>(30), resume.get_first(), true);
                    }
                }
            }

            if (Settings().Bool("MAIN.SkipLoadingScreens"))
            {
                pattern = iii ? hook::pattern("53 83 EC 50") : hook::pattern("53 83 EC 68 ? ? ? ? ? E8");
                if (!pattern.empty())
                    injector::WriteMemory<uint8_t>(pattern.get_first(), 0xC3, true);
            }

            if (!iii && Settings().Bool("MAIN.SkipOutro"))
            {
                auto outro = hook::pattern("83 3D ? ? ? ? 00 75 10");
                auto quit = hook::pattern("6A 00 6A 1E E8 ? ? ? ? 59 59");
                if (!outro.empty() && !quit.empty())
                    injector::MakeJMP(outro.get_first(), quit.get_first(), true);
            }

            if (loadSlot == LoadOff || !saveNamePrefix)
                return;
            auto idle = hook::pattern("E8 ? ? ? ? 83 C4 08 B8 01 00 00 00 5B C3");
            auto checkSlot = hook::pattern("8B 44 24 04 C7 05 ? ? ? ? 00 00 00 00 68 ? ? ? ? 50");
            auto menu = iii ? hook::pattern("? B9 ? ? ? ? C6 05 ? ? ? ? 01 E8 ? ? ? ? B9 ? ? ? ? C6") : hook::pattern("53 B9 ? ? ? ? 83 EC 28 68 ? ? ? ? 68");
            if (idle.size() <= size_t(iii ? 2 : 3) || checkSlot.empty() || menu.empty())
                return;
            if (iii)
            {
                // A new game from here leaves the emergency vehicles of the menu's last state: the menu field the
                // game clears is switched until the next frame of the world
                auto flag = hook::pattern("C6 85 ? 01 00 00 00 E8 ? ? ? ? 6A");
                auto vehicles = hook::pattern("B9 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 43 83 FB 32");
                if (!flag.empty() && !vehicles.empty())
                {
                    newGameMenuFlag.SetAddress(flag.get_first(2));
                    newGameHook = safetyhook::create_mid(vehicles.get_first(), [](SafetyHookContext&)
                    {
                        static bool done = false;
                        if (done)
                            return;
                        done = true;
                        newGameMenuFlag = 0x14;
                        At<uint8_t>(menuManager, 0x111) = 0;
                    }, safetyhook::MidHook::StartDisabled);
                }
            }
            else
            {
                pattern = hook::pattern("80 3D ? ? ? ? 00 53 89 CB ? ? ? ? ? ? 80");
                if (pattern.empty())
                    return;
                processOnOffMenuOptions = pattern.get_first<void __fastcall(uintptr_t)>();
            }
            menuManager = *menu.get_first<uintptr_t>(2);
            checkSlotDataValid = checkSlot.get_first<bool __cdecl(int)>();
            frontendIdleCall = reinterpret_cast<uintptr_t>(idle.get(iii ? 2 : 3).get<void>());
            frontendIdle = reinterpret_cast<decltype(frontendIdle)>(injector::GetBranchDestination(frontendIdleCall, true).as_int());
            injector::MakeCALL(frontendIdleCall, FrontendIdle, true);
        }
    }

    // ---------------------------------------------------------------------------------------------------------------
    // San Andreas

    namespace SA
    {
        uintptr_t frontendIdleCall = 0;
        void(__cdecl* frontendIdle)() = nullptr;
        uintptr_t cameraUpdateCall = 0;
        uintptr_t cameraUpdate = 0;
        bool(__cdecl* checkSlotDataValid)(int) = nullptr;
        uintptr_t menuManager = 0;
        uint8_t* missionPack = nullptr;
        uint8_t* userPause = nullptr;

        int __cdecl NoCameraUpdate()
        {
            return 0;
        }

        void __cdecl FrontendIdle()
        {
            // The menu isn't drawn on the first frames; the load starts on the second one
            static int frames = 0;
            if (++frames >= 2)
            {
                if (!skipLoad)
                {
                    int slot = ChooseSlot(checkSlotDataValid);
                    if (slot == LoadNewGame)
                    {
                        At<uint8_t>(menuManager, 0x5D) = 1;     // start a new game
                        At<uint8_t>(menuManager, 0x5C) = 0;     // menu inactive
                    }
                    else
                    {
                        // as if the player had chosen Load Game and the slot
                        At<uint8_t>(menuManager, 0x32) = 0;
                        At<uint8_t>(menuManager, 0x15F) = uint8_t(slot - 1);
                        *missionPack = 0;
                        At<uint8_t>(menuManager, 0x15D) = 13;
                        At<uint8_t>(menuManager, 0x1B3C) = 1;
                    }
                    *userPause = 0;
                }
                injector::MakeCALL(cameraUpdateCall, cameraUpdate, true);
                injector::MakeCALL(frontendIdleCall, frontendIdle, true);
            }
            frontendIdle();
        }

        void Init()
        {
            auto pattern = hook::pattern("68 ? ? ? ? 50 68 ? ? ? ? 68 ? ? ? ? E8 ? ? ? ? 83 C4 10 C3");
            if (!pattern.empty())
                saveNamePrefix = *pattern.get_first<char*>(12);

            if (*userFilesFolder)
            {
                auto a = hook::pattern("E8 ? ? ? ? BA ? ? ? ? 8D 9B 00 00 00 00 8A 08 40 88 0A");
                auto b = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? 6A 00 E8 ? ? ? ? 83 C4 08 84 C0 0F 95 C0");
                auto c = hook::pattern("E8 ? ? ? ? 50 E8 ? ? ? ? 83 C4 04 E8 ? ? ? ? 89 35 ? ? ? ? B8 01 00 00 00 0F A2");
                if (!a.empty() && !b.empty() && !c.empty())
                {
                    injector::MakeCALL(a.get_first(), UserFilesFolder, true);
                    injector::MakeCALL(b.get_first(), UserFilesFolder, true);
                    injector::MakeCALL(c.get_first(), UserFilesFolder, true);
                }
            }

            if (Settings().Bool("MAIN.SkipLoadingScreens"))
            {
                auto fade = hook::pattern("BE 14 00 00 00 8D A4 24 00 00 00 00 A1 ? ? ? ? 50 88 1D");
                auto fadeEnd = hook::pattern("3B FB 88 1D ? ? ? ? 88 1D ? ? ? ? 74 4B 88 1D");
                auto bar = hook::pattern("84 C9 74 05 83 F8 01 74 05 E8 ? ? ? ? 8B 15 ? ? ? ? 52 E8 ? ? ? ? A1 ? ? ? ? 50 E8 ? ? ? ? 83 C4 08 C3");
                auto screen = hook::pattern("E9 ? ? ? ? D8 1D ? ? ? ? DF E0 F6 C4 01 75 EB 6A 00 E8");
                auto tune = hook::pattern("E8 ? ? ? ? E8 ? ? ? ? B9 ? ? ? ? C7 05 ? ? ? ? 09 00 00 00");
                if (!fade.empty() && !fadeEnd.empty() && !bar.empty() && !screen.empty() && !tune.empty())
                {
                    injector::MakeJMP(fade.get_first(), fadeEnd.get_first(), true);     // no fading screens
                    injector::MakeNOP(bar.get_first(9), 5, true);                       // no loading bar
                    injector::MakeNOP(screen.get_first(), 5, true);                     // no loading screen
                    injector::WriteMemory<uint8_t>(screen.get_first(), 0xC3, true);
                    injector::MakeNOP(tune.get_first(), 5, true);                       // no loading screen music
                }
            }

            if (loadSlot == LoadOff || !saveNamePrefix)
                return;
            auto minimized = hook::pattern("74 0B 53 6A 1B E8 ? ? ? ? 83 C4 08 38 1D");
            auto idle = hook::pattern("E8 ? ? ? ? B8 01 00 00 00 C3 B8 02 00 00 00 C3");
            auto camera = hook::pattern("E8 ? ? ? ? 83 C4 24 85 C0 74 76 E8 ? ? ? ? A0 ? ? ? ? 84 C0 74 0A B9");
            auto checkSlot = hook::pattern("81 EC 04 01 00 00 56 8B B4 24 0C 01 00 00 8D 44 24 04 50 56 B9");
            auto mission = hook::pattern("8B CE 88 15 ? ? ? ? E8 ? ? ? ? C6 46 32");
            auto pause = hook::pattern("51 A0 ? ? ? ? 84 C0 56 57 8B F1 75 1B");
            if (minimized.empty() || idle.empty() || camera.empty() || checkSlot.empty() || mission.empty() || pause.empty())
                return;

            checkSlotDataValid = checkSlot.get_first<bool __cdecl(int)>();
            missionPack = *mission.get_first<uint8_t*>(4);
            userPause = *pause.get_first<uint8_t*>(2);
            menuManager = *camera.get_first<uintptr_t>(27);
            cameraUpdateCall = reinterpret_cast<uintptr_t>(camera.get_first());
            cameraUpdate = injector::GetBranchDestination(cameraUpdateCall, true).as_int();
            frontendIdleCall = reinterpret_cast<uintptr_t>(idle.get_first());
            frontendIdle = reinterpret_cast<decltype(frontendIdle)>(injector::GetBranchDestination(frontendIdleCall, true).as_int());
            injector::MakeNOP(minimized.get_first(), 2, true);                  // the menu also runs while minimized
            injector::MakeCALL(cameraUpdateCall, NoCameraUpdate, true);         // nothing drawn until the load starts
            injector::MakeCALL(frontendIdleCall, FrontendIdle, true);
        }
    }

    void Init()
    {
        auto& settings = Settings();
        loadSlot = std::clamp(settings.Int("MAIN.LoadSlot"), LoadOff, LoadNewGame);
        skipLoad = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;

        // A folder in the game's folder for the user files
        auto& folder = settings.String("MAIN.UserFilesFolder");
        if (!folder.empty())
        {
            char exe[MAX_PATH] = {};
            GetModuleFileNameA(nullptr, exe, MAX_PATH);
            auto path = std::filesystem::path(exe).parent_path() / std::filesystem::path(std::u8string(folder.begin(), folder.end()));
            strncpy_s(userFilesFolder, path.string().c_str(), _TRUNCATE);
        }

        // Patterns missing in an executable leave out only the features that need them
        if (!hook::pattern("E8 ? ? ? ? B8 01 00 00 00 C3 B8 02 00 00 00 C3").empty())
        {
            game = Game::SA;
            SA::Init();
        }
        else if (!hook::pattern("80 3D ? ? ? ? 00 53 89 CB ? ? ? ? ? ? 80").empty())
        {
            game = Game::VC;
            RW3::Init();
        }
        else
        {
            game = Game::III;
            RW3::Init();
        }
    }
}

extern "C" __declspec(dllexport) void InitializeASI()
{
    static std::once_flag flag;
    std::call_once(flag, []()
    {
        Settings();
        asi_settings::LiveLink::Start();
        // Steam executables are unpacked later: wait for their code
        CallbackHandler::RegisterCallbackAtGetSystemTimeAsFileTime(Init, hook::pattern("68 00 00 CF 00 ? ? 6A 00 FF 15"));
    });
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(module);
        InitializeASI();
    }
    return TRUE;
}

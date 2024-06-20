// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "SDK.h"
#include "finders.h"

bool (*ReadyToStartMatchOG)(UObject*);
bool ReadyToStartMatch(UObject* GM) {
	static bool bSetPlaylist = false;
	static bool bInit = false;

	if (!bSetPlaylist) {
		bSetPlaylist = true;
		GM->Get<uint32>("WarmupRequiredPlayerCount") = 4;

		auto Playlist = (UObject *) TUObjectArray::FindObject("Playlist_DefaultSolo");

		if (FNVer >= 6.10) {
			auto CurrentPlaylistInfo = GameState->GetPtr<FFastArraySerializer>("CurrentPlaylistInfo");

			StructGet(CurrentPlaylistInfo, "PlaylistPropertyArray", "BasePlaylist") = Playlist;
			StructGet(CurrentPlaylistInfo, "PlaylistPropertyArray", "OverridePlaylist") = Playlist;
			StructGet<uint32>(CurrentPlaylistInfo, "PlaylistPropertyArray", "PlaylistReplicationKey")++;
			CurrentPlaylistInfo->MarkArrayDirty();
			GameState->Call("OnRep_CurrentPlaylistInfo", nullptr);

			GameState->Get<uint32>("CurrentPlaylistId") = Playlist->Get<uint32>("PlaylistId");
			GameState->Call("OnRep_CurrentPlaylistId", nullptr);

			GM->Get<uint32>("CurrentPlaylistId") = Playlist->Get<uint32>("PlaylistId");
			GM->Get<FName>("CurrentPlaylistName") = Playlist->Get<FName>("PlaylistName");

			/*struct FAdditionalLevelStreamed
			{
			public:
				class FName LevelName;
				bool bIsServerOnly;
				uint8 _Padding1[0x3];
			};

			struct LevelStreamingDynamic_LoadLevelInstanceBySoftObjectPtr
			{
			public:
				class UObject* WorldContextObject;
				TSoftObjectPtr<UObject> Level;
				struct FVector Location;
				struct FRotator Rotation;
				bool bOutSuccess;
				uint8 _Padding1[0x7];
				class FString OptionalLevelNameOverride;
				class ULevelStreamingDynamic* ReturnValue;
			};

			auto AdditionalLevelsOff = Playlist->GetOffset("AdditionalLevels");
			auto AdditionalLevelsServerOnlyOff = Playlist->GetOffset("AdditionalLevelsServerOnly");

			if (AdditionalLevelsServerOnlyOff) {
				auto& AdditionalLevels = GetFromOffset<TArray<TSoftObjectPtr<UObject>>>(Playlist, AdditionalLevelsServerOnlyOff);

				for (auto& Level : AdditionalLevels) {
					bool Success = false;
					LevelStreamingDynamic_LoadLevelInstanceBySoftObjectPtr Params;
					Params.WorldContextObject = World;
					Params.Level = Level;
					Params.bOutSuccess = &Success;
					Params.Location = FVector{};
					Params.Rotation = FRotator{};
					Params.OptionalLevelNameOverride = L"";
					GetDefaultObj<"LevelStreamingDynamic">()->Call("LoadLevelInstanceBySoftObjectPtr", &Params);
					if (Success) GameState->Get<TArray<FAdditionalLevelStreamed>>("AdditionalPlaylistLevelsStreamed").Add({ Level.ObjectID.AssetPathName, true });
				}
			}
			if (AdditionalLevelsOff) {
				auto& AdditionalLevels = GetFromOffset<TArray<TSoftObjectPtr<UObject>>>(Playlist, AdditionalLevelsOff);

				for (auto& Level : AdditionalLevels) {
					bool Success = false;
					LevelStreamingDynamic_LoadLevelInstanceBySoftObjectPtr Params;
					Params.WorldContextObject = World;
					Params.Level = Level;
					Params.bOutSuccess = &Success;
					Params.Location = FVector{};
					Params.Rotation = FRotator{};
					Params.OptionalLevelNameOverride = L"";
					GetDefaultObj<"LevelStreamingDynamic">()->Call("LoadLevelInstanceBySoftObjectPtr", &Params);
					if (Success && AdditionalLevelsServerOnlyOff) GameState->Get<TArray<FAdditionalLevelStreamed>>("AdditionalPlaylistLevelsStreamed").Add({ Level.ObjectID.AssetPathName, false });
					else if (Success) GameState->Get<TArray<FName>>("AdditionalPlaylistLevelsStreamed").Add(Level.ObjectID.AssetPathName);
				}
			}*/
		}
		else if (FNVer > 4.0) {
			GameState->Get("CurrentPlaylistData") = Playlist;
		}
		else {
			GameState->Get<uint32>("CurrentPlaylistId") = 0;
		}
	}

	auto WarmupStarts = GetAll<"FortPlayerStartWarmup">();
	auto WarmupCount = WarmupStarts.Num();
	WarmupStarts.Free();
	if (WarmupCount == 0) return false;

	if (!bInit) {
		bInit = true;

		auto Beacon = SpawnActor(TUObjectArray::FindObject<UClass>("FortOnlineBeaconHost"), FVector{}, FRotator{});
		Beacon->Get<uint32>("ListenPort") = FNVer < 13.00 ? 43578 - 1 : 43578; // the beacon really doesnt matter much other than getting a netdriver, so it can be on any random port

		((bool (*)(AActor*)) FindInitHost())(Beacon);
		((void (*)(AActor*, bool)) FindPauseBeaconRequests())(Beacon, false);

		auto NetDriver = Beacon->Get("NetDriver");

		World->Get("NetDriver") = NetDriver;
		NetDriver->Get("World") = World;
		NetDriver->Get<FName>("NetDriverName") = Conv_StringToName(L"GameNetDriver");
		
		GM->Get("GameSession")->Get<uint32>("MaxPlayers") = 100;

		auto& LC = World->Get<TArray<UScriptStruct>>("LevelCollections");
		auto LCStruct = TUObjectArray::FindObject<UClass>("LevelCollection");
		auto C1 = LC.GetPtr(0, LCStruct->GetPropertiesSize());
		auto C2 = LC.GetPtr(1, LCStruct->GetPropertiesSize());
		StructGet(C1, "LevelCollection", "NetDriver") = NetDriver;
		StructGet(C2, "LevelCollection", "NetDriver") = NetDriver;

		FURL url;
		url.Port = 7777 - (FNVer >= 13.00 ? 1 : 0);
		FString Err;


		((void (*)(UObject*, UObject*)) FindSetWorld())(NetDriver, World);
		if (((bool (*)(UObject*, void*, FURL&, bool, FString&)) FindInitListen())(NetDriver, World, url, false, Err)) {
			GameMode->Get<bool>("bWorldIsReady") = true;

			Log("Listening on port 7777!");
		}
	}
	
	return ReadyToStartMatchOG(GM);
}

int NetMode() {
	return 1;
}

void (*TickFlushOG)(UObject*);
void TickFlush(UObject* NetDriver) {
	auto ReplicationDriver = NetDriver->Get("ReplicationDriver");
	if (ReplicationDriver) {
		static int VTOff = 0;
		if (!VTOff) {
			if (FNVer >= 2.5 && FNVer <= 4.5) VTOff = 0x53;
			else if (std::floor(FNVer) == 5) VTOff = 0x54;
			else if (FNVer >= 7.40 && FNVer < 8.40) VTOff = 0x57;
			else if (std::floor(FNVer) == 6 || std::floor(FNVer) >= 7 && std::floor(FNVer) <= 10) VTOff = 0x56;
			else if (FNVer >= 11 && FNVer <= 11.10) VTOff = 0x57;
			else if (FNVer == 11.30 || FNVer == 11.31) VTOff = 0x59;
			else if (std::floor(FNVer) == 11) VTOff = 0x5A;
			else if (std::floor(FNVer) == 12 || std::floor(FNVer) == 13) VTOff = 0x5D;
			else if (std::floor(FNVer) == 14 || FNVer <= 15.2) VTOff = 0x5E;
			else if (FNVer >= 15.3 && FNVer < 19) VTOff = 0x5F;
			else if (FNVer >= 19 && std::floor(FNVer) <= 20) VTOff = 0x66;
			else if (FNVer >= 21) VTOff = 0x67;
		}

		((void (*)(UObject*)) ReplicationDriver->Vft[VTOff])(ReplicationDriver);
	}
	else {
		// ew legacy replication
	}
	return TickFlushOG(NetDriver);
}

void InitNullsAndRetTrues() {
	if (FNVer == 0) NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 54 24 ? 48 89 4C 24 ? 55 53 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 8B 41 08 C1 E8 05").Get());
	else if (FNVer >= 3.3 && FNVer <= 4.5) NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 57 48 81 EC ? ? ? ? 4C 8B 82 ? ? ? ? 48 8B F9 0F 29 70 E8 0F 29 78 D8").Get());
	else if (FNVer == 4.1) NullFuncs.push_back(Memcury::Scanner::FindPattern("4C 8B DC 55 49 8D AB ? ? ? ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 85 ? ? ? ? 49 89 5B 10 48 8D 05 ? ? ? ? 48 8B 1D ? ? ? ? 49 89 73 18 33 F6 40").Get());
	else if (FNVer >= 5.00) {
		NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 70 10 57 48 81 EC ? ? ? ? 48 8B BA ? ? ? ? 48 8B DA 0F 29").Get());
		NullFuncs.push_back(Memcury::Scanner::FindStringRef(L"Widget Class %s - Running Initialize On Archetype, %s.").ScanFor(FNVer < 6.3 ? std::vector<uint8_t>{ 0x40, 0x55 } : std::vector<uint8_t>{ 0x48, 0x89, 0x5C }, false).Get());
	}
	else if (FNVer >= 7.00 && FNVer <= 12.00) NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 30 48 8B 41 28 48 8B DA 48 8B F9 48 85 C0 74 34 48 8B 4B 08 48 8D").Get());
	else if (FNVer >= 12.21 && FNVer < 13.00) {
		NullFuncs.push_back(Memcury::Scanner::FindPattern(FNVer == 12.41 ? "40 57 41 56 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F1 74 3A 80 3D ? ? ? ? ? 0F" : "40 57 41 56 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F1 74 3A 80 3D ? ? ? ? ? 0F 82").Get());
		if (FNVer == 12.41) NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 48 89 68 10 48 89 70 18 48 89 78 20 41 54 41 56 41 57 48 83 EC 70 48 8B 35").Get());
		else if (FNVer == 12.61) NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 57 41 54 41 56 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B 05 ? ? ? ? 48 33 C4 48 89 45 20 4C 8B A5").Get());
	}
	else if (FNVer == 14.60) NullFuncs.push_back(Memcury::Scanner::FindPattern("40 55 57 41 57 48 8D 6C 24 ? 48 81 EC ? ? ? ? 80 3D ? ? ? ? ? 0F B6 FA 44 8B F9 74 3B 80 3D ? ? ? ? ? 0F").Get());
	else if (FNVer >= 17.00) {
		NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 10 48 89 6C 24 20 56 57 41 54 41 56 41 57 48 81 EC ? ? ? ? 65 48 8B 04 25 ? ? ? ? 4C 8B F9").Get());
		if (std::floor(FNVer) == 17) NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 70 08 48 89 78 10 55 41 54 41 55 41 56 41 57 48 8D 68 A1 48 81 EC ? ? ? ? 45 33 ED").Get());
		else if (FNVer >= 19.00) NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 55 41 56 48 8B EC 48 83 EC 50 83 65 28 00 40 B6 05 40 38 35 ? ? ? ? 4C").Get());
		else if (FNVer >= 20.00) {
			NullFuncs.push_back(Memcury::Scanner::FindPattern("48 8B C4 48 89 58 08 4C 89 40 18 48 89 50 10 55 56 57 41 54 41 55 41 56 41 57 48 8D 68 98 48 81 EC ? ? ? ? 49 8B 48 20 45 33").Get());
			NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 57 48 83 EC 20 48 8B 41 20 48 8B FA 48 8B D9 BA ? ? ? ? 83 78 08 03 0F 8D").Get());
			NullFuncs.push_back(Memcury::Scanner::FindPattern("4C 89 44 24 ? 53 55 56 57 41 54 41 55 41 56 41 57 48 83 EC 68 48 8D 05 ? ? ? ? 0F").Get());
			NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC 30 48 8B F9 48 8B CA E8").Get());
			NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 41 ? 41 ? 48 83 EC 60 45 33 F6 4C 8D ? ? ? ? ? 48 8B DA").Get());
		}
	}

	if (FNVer >= 19.00) NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 55 56 57 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 50 4C 8B FA 48 8B F1 E8").Get());
	else if (FNVer >= 16.40) NullFuncs.push_back(Memcury::Scanner::FindPattern("48 89 5C 24 ? 48 89 74 24 ? 48 89 7C 24 ? 55 41 54 41 55 41 56 41 57 48 8B EC 48 83 EC 70 4C 8B FA 4C").Get());
	else if (FNVer == 2.5) NullFuncs.push_back(Memcury::Scanner::FindPattern("40 55 56 41 56 48 8B EC 48 81 EC ? ? ? ? 48 8B 01 4C 8B F2").Get());

	auto sRef = Memcury::Scanner::FindStringRef(L"Changing GameSessionId from '%s' to '%s'");
	NullFuncs.push_back(sRef.ScanFor({ 0x40, 0x55 }, false, 0, 1, 2000).Get());
}

float GetMaxTickRate() {
	return 30.0f;
}

void Main() {
	AllocConsole();
	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
	printf("LogUni: Display: Fortnite version: %.02f\n", FNVer);
	Log("Image base: ", ImageBase);

	MH_Initialize();
	Hook<"FortGameModeAthena", "ReadyToStartMatch">(ReadyToStartMatch, ReadyToStartMatchOG);
	Hook(FindWorldNetMode(), NetMode);
	Hook(FindTickFlush(), TickFlush, TickFlushOG);
	Hook(FindGetMaxTickRate(), GetMaxTickRate);
	MH_EnableHook(MH_ALL_HOOKS);

	InitNullsAndRetTrues();
	ProcessNullsAndRetTrues();


	//*(bool*)FindGIsClient() = false;
	World->Get("OwningGameInstance")->Get<TArray<UObject*>>("LocalPlayers").Remove(0);
	
	auto KSL = GetDefaultObj<"KismetSystemLibrary">();

	const wchar_t* terrainOpen = nullptr;
	switch (Chapter) {
	case 5:
		terrainOpen = L"open Helios_Terrain";
		break;
	case 4:
		terrainOpen = L"open Asteria_Terrain";
		break;
	case 3:
		terrainOpen = L"open Artemis_Terrain";
		break;
	case 2:
		terrainOpen = L"open Apollo_Terrain";
		break;
	case 1:
		terrainOpen = L"open Athena_Terrain";
		break;
	}
	struct KismetSystemLibrary_ExecuteConsoleCommand
	{
	public:
		class UObject* WorldContextObject;
		class FString Command;
		class UObject* SpecificPlayer;
	} Params {
		World, terrainOpen, nullptr
	};
	KSL->Call("ExecuteConsoleCommand", &Params);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
        thread(Main).detach();
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}


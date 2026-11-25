#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <tlhelp32.h>
#include <intrin.h>

#include <atomic>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <utility>

#include "MinHook.h"

namespace {

constexpr DWORD kExpectedTimeDateStamp = 0x684DE875;
constexpr DWORD kExpectedSizeOfImage = 0x2054C000;
constexpr ULONGLONG kExpectedFileSize = 528043656ULL;

constexpr uintptr_t kRvaFactoryAimTarget = 0x2ABD5C0;
constexpr uintptr_t kRvaFactoryDamageDealt = 0x2ABC670;
constexpr uintptr_t kRvaFactoryIsHostile = 0x2413F70;
constexpr uintptr_t kRvaFactoryGraphicsLines = 0x1B8B9B0;
constexpr uintptr_t kRvaFactoryGraphicsMultiSector = 0x1B8BC60;
constexpr uintptr_t kRvaFactoryInteractionListener = 0x425EA20;
constexpr uintptr_t kRvaFactoryInteractionModifier = 0x425EA80;
constexpr uintptr_t kRvaFactoryUIExecutionState = 0x4263EE0;
constexpr uintptr_t kRvaFactoryMenuGraphStackOnDemand = 0x4711810;
constexpr uintptr_t kRvaFactoryUISetExecutionState = 0x471E700;

constexpr uintptr_t kRvaEvalAimTarget = 0x2B64160;
constexpr uintptr_t kRvaEvalIsHostile = 0x24459D0;
constexpr uintptr_t kRvaEvalGraphicsBase = 0x1BA8010;
constexpr uintptr_t kRvaEvalInteractionListener = 0x42DEFF0;
constexpr uintptr_t kRvaEvalInteractionModifier = 0x42DFB50;
constexpr uintptr_t kRvaEvalUIExecutionState = 0x42FC620;
constexpr uintptr_t kRvaEvalMenuGraphStackOnDemand = 0x47B4060;
constexpr uintptr_t kRvaEvalUISetExecutionState = 0x47C4720;
constexpr uintptr_t kRvaUIExecutionStateManagerSet = 0x4A48240;
constexpr uintptr_t kRvaUIExecutionStateBoolCallback = 0x41268D0;
constexpr uintptr_t kRvaSetupInteractionListener = 0x4425DF0;
constexpr uintptr_t kRvaDestroyInteractionCallback = 0x41BFDB0;
constexpr uintptr_t kRvaSequenceLevelEntityToArchive = 0x4391EB0;
constexpr uintptr_t kRvaSequenceLevelEntityFromArchive = 0x44EA970;
constexpr uintptr_t kRvaSequenceBlockCollectionLoad = 0xC82030;
constexpr uintptr_t kRvaTargetTrackFromArchive = 0x1B12D60;
constexpr uintptr_t kRvaNixBowlGraphDispatch = 0x5B1AD0;
constexpr uintptr_t kRvaNixBowlOpenReturn = 0x42F8709;
constexpr uintptr_t kRvaNixBowlCloseReturn = 0x42F8AA3;
constexpr uintptr_t kRvaNixBowlInnerEvaluator = 0xC1DA30;
constexpr uintptr_t kRvaNixBowlInnerEvaluatorReturn = 0xC1DDEA;

constexpr uintptr_t kRvaGetBoolStorage = 0x0C3460;
constexpr uintptr_t kRvaGetFloatStorage = 0x0C20E0;
constexpr uintptr_t kRvaGetVec3Storage = 0x18A210;
constexpr uintptr_t kRvaGetDamageU32AStorage = 0x28632D0;
constexpr uintptr_t kRvaGetDamageU32BStorage = 0x2214820;
constexpr uintptr_t kRvaGetDamageRefAStorage = 0x2223840;
constexpr uintptr_t kRvaGetDamageRefBStorage = 0x285CEF0;
constexpr uintptr_t kRvaGetDamageRefCStorage = 0x337000;
constexpr uintptr_t kRvaGetDamageU32CStorage = 0x22136E0;

constexpr uintptr_t kRvaVtableAimTarget = 0x6022108;
constexpr uintptr_t kRvaVtableIsHostile = 0x5E4AD78;
constexpr uintptr_t kRvaVtableGraphicsLines = 0x5D19B28;
constexpr uintptr_t kRvaVtableGraphicsMultiSector = 0x5D1A6A0;
constexpr uintptr_t kRvaVtableInteractionListener = 0x6575448;
constexpr uintptr_t kRvaVtableInteractionModifier = 0x6576398;
constexpr uintptr_t kRvaVtableUIExecutionState = 0x65898B0;
constexpr uintptr_t kRvaVtableInteractionCallback = 0x6578630;
constexpr uintptr_t kRvaInteractionCallbackSignal = 0x1485F0;

constexpr size_t kEvalVtableSlot = 0x58 / sizeof(void*);
constexpr size_t kMaxTrackedInstances = 2048;
constexpr size_t kMaxTrackedDamageSlots = 8192;
constexpr size_t kMaxConsumerSeen = 8192;
constexpr size_t kMaxUISetExecutionEntries = 2048;
constexpr size_t kMaxCouchComponents = 32;
constexpr size_t kMaxUidWatchTargets = 4;
constexpr size_t kMaxUidWatchThreads = 512;
constexpr size_t kMaxUidWatchHits = 128;
constexpr size_t kMaxCouchThreadMarkers = 512;
constexpr size_t kUidWatchStackWords = 12;
constexpr uint32_t kContractsTerminalStateId = 38;
constexpr uint32_t kHideLocalPlayerStateId = 13;

struct Config {
    bool enabled = true;
    bool exactBuildOnly = true;
    bool captureOnStart = false;
    bool hookFactories = true;
    bool hookAimTarget = true;
    bool hookIsHostile = true;
    bool hookGraphics = true;
    bool hookDamageDealt = true;
    bool hookInteractionLifecycle = true;
    bool hookInteractionCallbacks = true;
    bool hookUIExecutionState = true;
    bool hookMenuGraphStackOnDemand = true;
    bool hookUISetExecutionState = true;
    bool hookUIExecutionStateManager = true;
    bool hookSequenceLevelEntity = true;
    bool hookSequenceBlockCollectionLoad = true;
    bool hookTargetTrackFromArchive = true;
    bool hookNixBowlGraphDispatch = false;
    bool hookNixBowlInnerEvaluator = false;
    bool couchUidWatch = true;
    bool overlayNotifications = true;
    bool audioMarker = true;
    bool rawEvaluatorCalls = false;
    bool interactionEvaluatorCalls = true;
    bool interactionTransitionDetails = false;
    bool interactionObservedStateDetails = false;
    bool consumerTrace = false;
    bool gateHasDealtDamage = false;
    bool gateFatal = false;
    bool automaticSnapshots = true;
    char testLabel[128] = "unlabeled";
    DWORD startupDelayMs = 500;
    DWORD consumerTraceWindowMs = 60;
    uint64_t maxEvents = 250000;
    uint64_t maxConsumerEvents = 25000;
};

enum class StorageKind : uint8_t {
    Bool,
    Float,
    U32A,
    U32B,
    RefA,
    RefB,
    RefC,
    U32C,
};

enum class NodeKind : uint32_t {
    AimTarget,
    DamageDealt,
    IsHostile,
    GraphicsLines,
    GraphicsMultiSector,
    InteractionListener,
    InteractionModifier,
    UIExecutionState,
    MenuGraphStackOnDemand,
    UISetExecutionState,
};

struct InstanceSet {
    SRWLOCK lock = SRWLOCK_INIT;
    void* items[kMaxTrackedInstances] = {};
    size_t count = 0;
};

struct AimSnapshot {
    uint32_t readableMask = 0;
    uint64_t output20 = 0;
    uint64_t output30 = 0;
    uint64_t output40 = 0;
    int32_t distanceHalfMeters = 0;
    uint8_t output28 = 0;
    uint8_t output48 = 0;
};

struct DamageSnapshot {
    uint32_t readableMask = 0;
    uint32_t output30 = 0;
    uint32_t output40 = 0;
    uint32_t output48 = 0;
    uint32_t output50 = 0;
    uint32_t output58 = 0;
    uint32_t output70 = 0;
    uint32_t output78 = 0;
    uint32_t output88 = 0;
    uint64_t output60 = 0;
    uint64_t output68 = 0;
    uint64_t output80 = 0;
    uint8_t output28 = 0;
    uint8_t output38 = 0;
    uint8_t output90 = 0;
};

struct HostileSnapshot {
    uint32_t readableMask = 0;
    uint8_t output20 = 0;
};

struct GraphicsSnapshot {
    uint32_t readableMask = 0;
    uint32_t output6C = 0;
    uint8_t nodeReady28 = 0;
    uint8_t contextStatus824 = 0;
    uint8_t output18 = 0;
};

struct InteractionSnapshot {
    uint32_t readableMask = 0;
    uint64_t input20 = 0;
    uint64_t runtime840 = 0;
    uint16_t executionState858 = 0;
};

struct OnDemandSnapshot {
    uint32_t readableMask = 0;
    uint64_t child58 = 0;
    uint64_t owner60 = 0;
    uint32_t state68 = 0;
    uint32_t pushType6C = 0;
};

struct UISetExecutionEntry {
    void* self = nullptr;
    void* context = nullptr;
    uint64_t firstUs = 0;
    uint64_t lastUs = 0;
    uint64_t callCount = 0;
    uintptr_t callerRva = 0;
    uint64_t stateObject28 = 0;
    uint64_t trailing38 = 0;
    uint32_t stateId30 = 0;
    uint32_t readableMask = 0;
};

struct UISetExecutionCache {
    SRWLOCK lock = SRWLOCK_INIT;
    UISetExecutionEntry entries[kMaxUISetExecutionEntries] = {};
    size_t count = 0;
};

struct DamageSlotEntry {
    void* slot = nullptr;
    void* self = nullptr;
    void* context = nullptr;
    uintptr_t getterRva = 0;
    uint32_t tag = 0;
    uint16_t slotOffset = 0;
    StorageKind kind = StorageKind::Bool;
};

struct ConsumerSeenEntry {
    uint64_t pulse = 0;
    void* slot = nullptr;
    uintptr_t callerRva = 0;
};

struct DamageSlotRegistry {
    SRWLOCK lock = SRWLOCK_INIT;
    DamageSlotEntry entries[kMaxTrackedDamageSlots] = {};
    size_t count = 0;
};

struct ConsumerSeenCache {
    SRWLOCK lock = SRWLOCK_INIT;
    ConsumerSeenEntry entries[kMaxConsumerSeen] = {};
    size_t count = 0;
};

struct CouchComponentEntry {
    void* component = nullptr;
    uint64_t seenSequence = 0;
    bool ownerPresent = false;
};

struct CouchComponentRegistry {
    SRWLOCK lock = SRWLOCK_INIT;
    CouchComponentEntry entries[kMaxCouchComponents] = {};
    size_t count = 0;
    uint64_t sequence = 0;
};

struct UidWatchTarget {
    void* component = nullptr;
    uintptr_t address = 0;
};

struct UidWatchThreadState {
    DWORD threadId = 0;
    DWORD64 dr0 = 0;
    DWORD64 dr1 = 0;
    DWORD64 dr2 = 0;
    DWORD64 dr3 = 0;
    DWORD64 dr6 = 0;
    DWORD64 dr7 = 0;
};

struct UidWatchHit {
    std::atomic<LONG> ready{0};
    DWORD threadId = 0;
    uint32_t slot = 0;
    uint64_t qpc = 0;
    uintptr_t component = 0;
    uintptr_t address = 0;
    uintptr_t nextRip = 0;
    uintptr_t rsp = 0;
    uintptr_t rbp = 0;
    uintptr_t rax = 0;
    uintptr_t rbx = 0;
    uintptr_t rcx = 0;
    uintptr_t rdx = 0;
    uintptr_t rsi = 0;
    uintptr_t rdi = 0;
    uintptr_t r8 = 0;
    uintptr_t r9 = 0;
    uintptr_t r10 = 0;
    uintptr_t r11 = 0;
    uintptr_t r12 = 0;
    uintptr_t r13 = 0;
    uintptr_t r14 = 0;
    uintptr_t r15 = 0;
    uintptr_t stackWords[kUidWatchStackWords] = {};
    uint8_t codeBefore[16] = {};
    uint8_t codeAfter[16] = {};
};

struct CouchThreadMarker {
    DWORD threadId = 0;
    uint64_t generation = 0;
    void* component = nullptr;
};

struct CouchThreadMarkerRegistry {
    SRWLOCK lock = SRWLOCK_INIT;
    CouchThreadMarker entries[kMaxCouchThreadMarkers] = {};
    size_t count = 0;
};

template <typename Snapshot>
struct StateCache {
    struct Entry {
        void* self = nullptr;
        uint32_t readableMask = 0;
        Snapshot snapshot = {};
    };
    SRWLOCK lock = SRWLOCK_INIT;
    Entry entries[kMaxTrackedInstances] = {};
    size_t count = 0;
};

using FactoryFn = void* (__fastcall*)();
using EvalFn = void (__fastcall*)(void*, void*);
using InteractionSetupFn = void (__fastcall*)(void*, void*);
using UIExecutionStateBoolTargetFn = void (__fastcall*)(void*, uint8_t);
using InteractionCallbackSignalFn = bool (__fastcall*)(void*, volatile LONG*);
using InteractionCallbackDestroyFn = void* (__fastcall*)(void*, uint32_t);
using UIExecutionStateManagerSetFn = void (__fastcall*)(void*, void*);
using SequenceLevelEntityArchiveFn = void* (__fastcall*)(void*, void*);
using SequenceBlockCollectionLoadFn = bool (__fastcall*)(void*, void*);
using TargetTrackFromArchiveFn = bool (__fastcall*)(void*, void*, void*);
using NixBowlGraphDispatchFn = bool (__fastcall*)(void*, void*, uint8_t);
using NixBowlInnerEvaluatorFn = bool (__fastcall*)(void*, void*, uint8_t, int32_t,
    int32_t, uint8_t);

HMODULE g_module = nullptr;
uintptr_t g_base = 0;
size_t g_imageSize = 0;
uintptr_t g_observerBase = 0;
size_t g_observerSize = 0;
char g_dllDir[MAX_PATH] = {};
char g_gameDir[MAX_PATH] = {};
char g_iniPath[MAX_PATH] = {};
char g_logPath[MAX_PATH] = {};
char g_eventPath[MAX_PATH] = {};
char g_statePath[MAX_PATH] = {};
char g_consumerPath[MAX_PATH] = {};
char g_snapshotPath[MAX_PATH] = {};
FILE* g_log = nullptr;
FILE* g_events = nullptr;
FILE* g_states = nullptr;
FILE* g_consumers = nullptr;
FILE* g_snapshots = nullptr;
CRITICAL_SECTION g_logLock;
bool g_logLockReady = false;
Config g_cfg;
LARGE_INTEGER g_qpcFrequency = {};
LARGE_INTEGER g_qpcStart = {};
std::atomic<bool> g_running{true};
std::atomic<bool> g_capture{false};
std::atomic<uint64_t> g_sequence{0};
std::atomic<uint64_t> g_dropped{0};
std::atomic<uint64_t> g_stateSequence{0};
std::atomic<uint64_t> g_consumerSequence{0};
std::atomic<uint64_t> g_consumerDropped{0};
std::atomic<uint64_t> g_snapshotSequence{0};
std::atomic<uint64_t> g_interactionCallbackSignals{0};
std::atomic<uint64_t> g_interactionCallbackDestroys{0};
std::atomic<uint64_t> g_interactionListenerTransitions{0};
std::atomic<uint64_t> g_interactionModifierTransitions{0};
std::atomic<uint64_t> g_interactionListenerObservations{0};
std::atomic<uint64_t> g_interactionModifierObservations{0};
std::atomic<uint64_t> g_nixBowlGraphOpenCalls{0};
std::atomic<uint64_t> g_nixBowlGraphCloseCalls{0};
std::atomic<uint64_t> g_nixBowlInnerOpenCalls{0};
std::atomic<uint64_t> g_nixBowlInnerCloseCalls{0};
std::atomic<uint64_t> g_nixBowlInnerCallerMismatches{0};
std::atomic<void*> g_pendingDamageEval{nullptr};
std::atomic<bool> g_damageEvalInstalled{false};
std::atomic<uint32_t> g_markerNumber{0};
std::atomic<uint64_t> g_damagePulseNumber{0};
std::atomic<uint64_t> g_lastDamagePulseUs{0};
std::atomic<uint64_t> g_consumerTraceUntilUs{0};
std::atomic<bool> g_contractsSetExecutionWindow{false};
std::atomic<uint64_t> g_contractsSetExecutionArmUs{0};
std::atomic<void*> g_contractsUIExecutionStateManager{nullptr};
std::atomic<uint32_t> g_contractsUIExecutionStatePresence{0xFFFFFFFFu};
std::atomic<bool> g_contractsUIExecutionStatesSeen{false};
std::atomic<bool> g_contractsUIExecutionStatesRetired{false};
std::atomic<bool> g_gateHasDealtDamage{false};
std::atomic<bool> g_gateFatal{false};
std::atomic<uint64_t> g_sequenceLevelEntityToArchiveCalls{0};
std::atomic<uint64_t> g_sequenceLevelEntityFromArchiveCalls{0};
std::atomic<uint64_t> g_sequenceLevelEntityCouchMatches{0};
std::atomic<bool> g_sequenceLevelEntityCouchLogged{false};
std::atomic<uint64_t> g_sequenceBlockCouchLoads{0};
std::atomic<uint64_t> g_targetTrackCouchLoads{0};
std::atomic<bool> g_uidWatchRequested{false};
std::atomic<bool> g_uidWatchArmed{false};
std::atomic<bool> g_uidWatchStopping{false};
std::atomic<bool> g_uidWatchStopRequested{false};
std::atomic<uint32_t> g_uidWatchWriteIndex{0};
std::atomic<uint32_t> g_uidWatchDropped{0};
std::atomic<uint32_t> g_uidWatchStaleIgnored{0};
std::atomic<uint64_t> g_uidWatchCandidateSequence{0};
uint32_t g_uidWatchDrainIndex = 0;
std::atomic<uint64_t> g_uidWatchArmedCandidateSequence{0};
UidWatchTarget g_uidWatchTargets[kMaxUidWatchTargets] = {};
size_t g_uidWatchTargetCount = 0;
UidWatchThreadState g_uidWatchThreads[kMaxUidWatchThreads] = {};
size_t g_uidWatchThreadCount = 0;
PVOID g_uidWatchVeh = nullptr;
void* g_consumerHookTargets[8] = {};
size_t g_consumerHookCount = 0;
bool g_consumerHooksEnabled = false;
HWND g_overlayWindow = nullptr;
char g_overlayText[128] = {};
ULONGLONG g_overlayHideAt = 0;

InstanceSet g_aimInstances;
InstanceSet g_damageInstances;
InstanceSet g_hostileInstances;
InstanceSet g_linesInstances;
InstanceSet g_multiInstances;
InstanceSet g_interactionListenerInstances;
InstanceSet g_interactionModifierInstances;
InstanceSet g_uiExecutionStateInstances;
InstanceSet g_menuGraphStackOnDemandInstances;
InstanceSet g_uiSetExecutionStateInstances;
InstanceSet g_interactionCallbackInstances;
StateCache<AimSnapshot> g_aimStates;
StateCache<DamageSnapshot> g_damageStates;
StateCache<HostileSnapshot> g_hostileStates;
StateCache<GraphicsSnapshot> g_lineStates;
StateCache<GraphicsSnapshot> g_multiStates;
StateCache<InteractionSnapshot> g_interactionListenerStates;
StateCache<InteractionSnapshot> g_interactionModifierStates;
StateCache<InteractionSnapshot> g_uiExecutionStateStates;
StateCache<OnDemandSnapshot> g_menuGraphStackOnDemandStates;
UISetExecutionCache g_uiSetExecutionStates;
DamageSlotRegistry g_damageSlots;
ConsumerSeenCache g_consumerSeen;
CouchComponentRegistry g_couchComponents;
CouchThreadMarkerRegistry g_couchThreadMarkers;
UidWatchHit g_uidWatchHits[kMaxUidWatchHits];

FactoryFn g_origAimFactory = nullptr;
FactoryFn g_origDamageFactory = nullptr;
FactoryFn g_origHostileFactory = nullptr;
FactoryFn g_origLinesFactory = nullptr;
FactoryFn g_origMultiFactory = nullptr;
FactoryFn g_origInteractionListenerFactory = nullptr;
FactoryFn g_origInteractionModifierFactory = nullptr;
FactoryFn g_origUIExecutionStateFactory = nullptr;
FactoryFn g_origMenuGraphStackOnDemandFactory = nullptr;
FactoryFn g_origUISetExecutionStateFactory = nullptr;
EvalFn g_origAimEval = nullptr;
EvalFn g_origHostileEval = nullptr;
EvalFn g_origGraphicsEval = nullptr;
EvalFn g_origDamageEval = nullptr;
EvalFn g_origInteractionListenerEval = nullptr;
EvalFn g_origInteractionModifierEval = nullptr;
EvalFn g_origUIExecutionStateEval = nullptr;
EvalFn g_origMenuGraphStackOnDemandEval = nullptr;
EvalFn g_origUISetExecutionStateEval = nullptr;
UIExecutionStateManagerSetFn g_origUIExecutionStateManagerSet = nullptr;
SequenceLevelEntityArchiveFn g_origSequenceLevelEntityToArchive = nullptr;
SequenceLevelEntityArchiveFn g_origSequenceLevelEntityFromArchive = nullptr;
SequenceBlockCollectionLoadFn g_origSequenceBlockCollectionLoad = nullptr;
TargetTrackFromArchiveFn g_origTargetTrackFromArchive = nullptr;
NixBowlGraphDispatchFn g_origNixBowlGraphDispatch = nullptr;
NixBowlInnerEvaluatorFn g_origNixBowlInnerEvaluator = nullptr;
InteractionSetupFn g_origInteractionListenerSetup = nullptr;
InteractionCallbackSignalFn g_origInteractionCallbackSignal = nullptr;
InteractionCallbackDestroyFn g_origInteractionCallbackDestroy = nullptr;
bool g_interactionCallbackVtablePatched = false;
bool g_uiExecutionStateBoolCallbackPatched = false;
using SlotGetterFn = void* (__fastcall*)(void*, uint32_t);
SlotGetterFn g_origGetBoolStorage = nullptr;
SlotGetterFn g_origGetFloatStorage = nullptr;
SlotGetterFn g_origGetDamageU32AStorage = nullptr;
SlotGetterFn g_origGetDamageU32BStorage = nullptr;
SlotGetterFn g_origGetDamageRefAStorage = nullptr;
SlotGetterFn g_origGetDamageRefBStorage = nullptr;
SlotGetterFn g_origGetDamageRefCStorage = nullptr;
SlotGetterFn g_origGetDamageU32CStorage = nullptr;

thread_local bool g_insideDamageEval = false;
thread_local bool g_observerStorageRead = false;
thread_local uint32_t g_nixBowlDispatchDepth = 0;
thread_local bool g_nixBowlDispatchOpen = false;
thread_local uint8_t g_nixBowlDispatchSelector = 0;
thread_local void* g_nixBowlSelectedEntry = nullptr;
thread_local uint32_t g_nixBowlInnerSeen = 0;
thread_local uintptr_t g_nixBowlInnerLastCallerRva = 0;

void Log(const char* format, ...);
void StateTrace(const char* node, void* self, void* context, const char* format, ...);
bool ReadVtable(void* object, void*** outVtable);

const char* KindName(NodeKind kind) {
    switch (kind) {
    case NodeKind::AimTarget: return "HC_CombatUINode_PlayerAimTarget";
    case NodeKind::DamageDealt: return "HC_CombatNode_DamageDealt";
    case NodeKind::IsHostile: return "HC_AgentNode_IsHostile";
    case NodeKind::GraphicsLines: return "UI:GraphicsLines";
    case NodeKind::GraphicsMultiSector: return "UI:GraphicsMultiSector";
    case NodeKind::InteractionListener: return "HC_ScriptInteractionListenerNode";
    case NodeKind::InteractionModifier: return "HC_ScriptInteractionModifierNode";
    case NodeKind::UIExecutionState: return "HC_ScriptUIExecutionStateListenerNode";
    case NodeKind::MenuGraphStackOnDemand: return "HC_UI:MenuGraphStack_OnDemand";
    case NodeKind::UISetExecutionState: return "UI:SetExecutionState";
    }
    return "unknown";
}

InstanceSet& SetFor(NodeKind kind) {
    switch (kind) {
    case NodeKind::AimTarget: return g_aimInstances;
    case NodeKind::DamageDealt: return g_damageInstances;
    case NodeKind::IsHostile: return g_hostileInstances;
    case NodeKind::GraphicsLines: return g_linesInstances;
    case NodeKind::InteractionListener: return g_interactionListenerInstances;
    case NodeKind::InteractionModifier: return g_interactionModifierInstances;
    case NodeKind::UIExecutionState: return g_uiExecutionStateInstances;
    case NodeKind::MenuGraphStackOnDemand: return g_menuGraphStackOnDemandInstances;
    case NodeKind::UISetExecutionState: return g_uiSetExecutionStateInstances;
    default: return g_multiInstances;
    }
}

void ResolvePaths(HMODULE module) {
    char dllPath[MAX_PATH] = {};
    GetModuleFileNameA(module, dllPath, MAX_PATH);
    strcpy_s(g_dllDir, dllPath);
    if (char* slash = strrchr(g_dllDir, '\\')) *slash = '\0';
    snprintf(g_iniPath, MAX_PATH, "%s\\outlaws_event_observer.ini", g_dllDir);

    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    strcpy_s(g_gameDir, exePath);
    if (char* slash = strrchr(g_gameDir, '\\')) *slash = '\0';
    snprintf(g_logPath, MAX_PATH, "%s\\outlaws_event_observer.log", g_gameDir);
    snprintf(g_eventPath, MAX_PATH, "%s\\outlaws_event_observer_events.tsv", g_gameDir);
    snprintf(g_statePath, MAX_PATH, "%s\\outlaws_event_observer_states.tsv", g_gameDir);
    snprintf(g_consumerPath, MAX_PATH, "%s\\outlaws_event_observer_consumers.tsv", g_gameDir);
    snprintf(g_snapshotPath, MAX_PATH, "%s\\outlaws_event_observer_snapshots.tsv", g_gameDir);
}

bool ReadBool(const char* section, const char* key, bool fallback) {
    char value[32] = {};
    GetPrivateProfileStringA(section, key, fallback ? "true" : "false",
        value, sizeof(value), g_iniPath);
    return _stricmp(value, "true") == 0 || strcmp(value, "1") == 0 ||
        _stricmp(value, "yes") == 0 || _stricmp(value, "on") == 0;
}

void ReadConfig() {
    g_cfg.enabled = ReadBool("General", "Enabled", true);
    g_cfg.exactBuildOnly = ReadBool("General", "ExactBuildOnly", true);
    g_cfg.captureOnStart = ReadBool("Capture", "CaptureOnStart", false);
    g_cfg.hookFactories = ReadBool("Hooks", "Factories", true);
    g_cfg.hookAimTarget = ReadBool("Hooks", "PlayerAimTarget", true);
    g_cfg.hookIsHostile = ReadBool("Hooks", "IsHostile", true);
    g_cfg.hookGraphics = ReadBool("Hooks", "Graphics", true);
    g_cfg.hookDamageDealt = ReadBool("Hooks", "DamageDealt", true);
    g_cfg.hookInteractionLifecycle = ReadBool("Hooks", "InteractionLifecycle", true);
    g_cfg.hookInteractionCallbacks = ReadBool("Hooks", "InteractionCallbacks", true);
    g_cfg.hookUIExecutionState = ReadBool("Hooks", "UIExecutionState", true);
    g_cfg.hookMenuGraphStackOnDemand = ReadBool(
        "Hooks", "MenuGraphStackOnDemand", true);
    g_cfg.hookUISetExecutionState = ReadBool(
        "Hooks", "UISetExecutionState", true);
    g_cfg.hookUIExecutionStateManager = ReadBool(
        "Hooks", "UIExecutionStateManager", true);
    g_cfg.hookSequenceLevelEntity = ReadBool(
        "Hooks", "SequenceLevelEntity", true);
    g_cfg.hookSequenceBlockCollectionLoad = ReadBool(
        "Hooks", "SequenceBlockCollectionLoad", true);
    g_cfg.hookTargetTrackFromArchive = ReadBool(
        "Hooks", "TargetTrackFromArchive", true);
    g_cfg.hookNixBowlGraphDispatch = ReadBool(
        "Hooks", "NixBowlGraphDispatch", false);
    g_cfg.hookNixBowlInnerEvaluator = ReadBool(
        "Hooks", "NixBowlInnerEvaluator", false);
    g_cfg.couchUidWatch = ReadBool("Capture", "CouchUidWatch", true);
    g_cfg.overlayNotifications = ReadBool("Display", "OverlayNotifications", true);
    g_cfg.audioMarker = ReadBool("Display", "AudioMarker", true);
    g_cfg.rawEvaluatorCalls = ReadBool("Capture", "RawEvaluatorCalls", false);
    g_cfg.interactionEvaluatorCalls = ReadBool("Capture", "InteractionEvaluatorCalls", true);
    g_cfg.interactionTransitionDetails = ReadBool(
        "Capture", "InteractionTransitionDetails", false);
    g_cfg.interactionObservedStateDetails = ReadBool(
        "Capture", "InteractionObservedStateDetails", false);
    g_cfg.consumerTrace = ReadBool("Capture", "ConsumerTrace", false);
    g_cfg.gateHasDealtDamage = ReadBool("DiagnosticGates", "HasDealtDamage", false);
    g_cfg.gateFatal = ReadBool("DiagnosticGates", "Fatal", false);
    g_cfg.automaticSnapshots = ReadBool("Session", "AutomaticSnapshots", true);
    GetPrivateProfileStringA("Session", "TestLabel", "unlabeled", g_cfg.testLabel,
        static_cast<DWORD>(sizeof(g_cfg.testLabel)), g_iniPath);
    for (char* cursor = g_cfg.testLabel; *cursor; ++cursor) {
        if (*cursor == '\t' || *cursor == '\r' || *cursor == '\n') *cursor = ' ';
    }
    g_cfg.startupDelayMs = static_cast<DWORD>(GetPrivateProfileIntA(
        "General", "StartupDelayMs", 500, g_iniPath));
    g_cfg.maxEvents = static_cast<uint64_t>(GetPrivateProfileIntA(
        "Capture", "MaxEvents", 250000, g_iniPath));
    g_cfg.consumerTraceWindowMs = static_cast<DWORD>(GetPrivateProfileIntA(
        "Capture", "ConsumerTraceWindowMs", 60, g_iniPath));
    g_cfg.maxConsumerEvents = static_cast<uint64_t>(GetPrivateProfileIntA(
        "Capture", "MaxConsumerEvents", 25000, g_iniPath));
}

BOOL CALLBACK FindGameWindowCallback(HWND window, LPARAM parameter) {
    if (window == g_overlayWindow || !IsWindowVisible(window) || GetWindow(window, GW_OWNER))
        return TRUE;
    DWORD processId = 0;
    GetWindowThreadProcessId(window, &processId);
    if (processId != GetCurrentProcessId()) return TRUE;
    RECT rect = {};
    if (!GetWindowRect(window, &rect)) return TRUE;
    const LONG area = (rect.right - rect.left) * (rect.bottom - rect.top);
    auto* result = reinterpret_cast<std::pair<HWND, LONG>*>(parameter);
    if (area > result->second) {
        result->first = window;
        result->second = area;
    }
    return TRUE;
}

HWND FindGameWindow() {
    std::pair<HWND, LONG> result = {nullptr, 0};
    EnumWindows(FindGameWindowCallback, reinterpret_cast<LPARAM>(&result));
    return result.first;
}

LRESULT CALLBACK OverlayWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    if (message == WM_ERASEBKGND) return 1;
    if (message == WM_PAINT) {
        PAINTSTRUCT paint = {};
        HDC dc = BeginPaint(window, &paint);
        RECT rect = {};
        GetClientRect(window, &rect);
        HBRUSH background = CreateSolidBrush(RGB(14, 18, 22));
        FillRect(dc, &rect, background);
        DeleteObject(background);
        SetBkMode(dc, TRANSPARENT);
        SetTextColor(dc, RGB(240, 245, 250));
        HFONT font = CreateFontA(25, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, "Segoe UI");
        HGDIOBJ oldFont = SelectObject(dc, font);
        DrawTextA(dc, g_overlayText, -1, &rect,
            DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        SelectObject(dc, oldFont);
        DeleteObject(font);
        EndPaint(window, &paint);
        return 0;
    }
    return DefWindowProcA(window, message, wParam, lParam);
}

void CreateOverlay() {
    if (!g_cfg.overlayNotifications || g_overlayWindow) return;
    WNDCLASSEXA windowClass = {};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = OverlayWindowProc;
    windowClass.hInstance = g_module;
    windowClass.lpszClassName = "OutlawsEventObserverOverlay";
    RegisterClassExA(&windowClass);
    g_overlayWindow = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED | WS_EX_TRANSPARENT,
        windowClass.lpszClassName, "Outlaws Event Observer", WS_POPUP,
        0, 0, 520, 54, nullptr, nullptr, g_module, nullptr);
    if (!g_overlayWindow) {
        Log("overlay notification window could not be created");
        return;
    }
    SetLayeredWindowAttributes(g_overlayWindow, 0, 225, LWA_ALPHA);
    Log("overlay notifications enabled");
}

void ShowOverlay(const char* text, DWORD durationMs = 1600) {
    if (!g_overlayWindow) return;
    strcpy_s(g_overlayText, text);
    RECT bounds = {};
    HWND gameWindow = FindGameWindow();
    if (!gameWindow || !GetWindowRect(gameWindow, &bounds)) {
        bounds.left = 0;
        bounds.top = 0;
        bounds.right = GetSystemMetrics(SM_CXSCREEN);
        bounds.bottom = GetSystemMetrics(SM_CYSCREEN);
    }
    constexpr int width = 520;
    constexpr int height = 54;
    const int x = bounds.left + ((bounds.right - bounds.left) - width) / 2;
    const int y = bounds.top + 72;
    SetWindowPos(g_overlayWindow, HWND_TOPMOST, x, y, width, height,
        SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(g_overlayWindow, nullptr, TRUE);
    UpdateWindow(g_overlayWindow);
    g_overlayHideAt = GetTickCount64() + durationMs;
}

void PumpOverlay() {
    if (!g_overlayWindow) return;
    MSG message = {};
    while (PeekMessageA(&message, g_overlayWindow, 0, 0, PM_REMOVE)) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
    if (g_overlayHideAt && GetTickCount64() >= g_overlayHideAt) {
        ShowWindow(g_overlayWindow, SW_HIDE);
        g_overlayHideAt = 0;
    }
}

void OpenLogs() {
    InitializeCriticalSection(&g_logLock);
    g_logLockReady = true;
    fopen_s(&g_log, g_logPath, "a");
    if (!g_log) {
        snprintf(g_logPath, MAX_PATH, "%s\\outlaws_event_observer.log", g_dllDir);
        fopen_s(&g_log, g_logPath, "a");
    }
    fopen_s(&g_events, g_eventPath, "a");
    if (!g_events) {
        snprintf(g_eventPath, MAX_PATH, "%s\\outlaws_event_observer_events.tsv", g_dllDir);
        fopen_s(&g_events, g_eventPath, "a");
    }
    fopen_s(&g_states, g_statePath, "a");
    if (!g_states) {
        snprintf(g_statePath, MAX_PATH, "%s\\outlaws_event_observer_states.tsv", g_dllDir);
        fopen_s(&g_states, g_statePath, "a");
    }
    fopen_s(&g_consumers, g_consumerPath, "a");
    if (!g_consumers) {
        snprintf(g_consumerPath, MAX_PATH, "%s\\outlaws_event_observer_consumers.tsv", g_dllDir);
        fopen_s(&g_consumers, g_consumerPath, "a");
    }
    fopen_s(&g_snapshots, g_snapshotPath, "a");
    if (!g_snapshots) {
        snprintf(g_snapshotPath, MAX_PATH, "%s\\outlaws_event_observer_snapshots.tsv", g_dllDir);
        fopen_s(&g_snapshots, g_snapshotPath, "a");
    }
    if (g_events) setvbuf(g_events, nullptr, _IOFBF, 64 * 1024);
    if (g_states) setvbuf(g_states, nullptr, _IOFBF, 64 * 1024);
    if (g_consumers) setvbuf(g_consumers, nullptr, _IOFBF, 64 * 1024);
    if (g_snapshots) setvbuf(g_snapshots, nullptr, _IOFBF, 64 * 1024);
}

void Log(const char* format, ...) {
    if (!g_log || !g_logLockReady) return;
    EnterCriticalSection(&g_logLock);
    SYSTEMTIME now = {};
    GetLocalTime(&now);
    fprintf(g_log, "[%02u:%02u:%02u.%03u] ", now.wHour, now.wMinute,
        now.wSecond, now.wMilliseconds);
    va_list args;
    va_start(args, format);
    vfprintf(g_log, format, args);
    va_end(args);
    fputc('\n', g_log);
    fflush(g_log);
    LeaveCriticalSection(&g_logLock);
}

uint64_t QpcMicroseconds() {
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    return static_cast<uint64_t>((now.QuadPart - g_qpcStart.QuadPart) * 1000000LL /
        g_qpcFrequency.QuadPart);
}

uintptr_t ToRva(const void* address) {
    const uintptr_t value = reinterpret_cast<uintptr_t>(address);
    if (value < g_base || value >= g_base + g_imageSize) return 0;
    return value - g_base;
}

void Trace(const char* eventName, void* self, void* context, void* caller) {
    if (!g_capture.load(std::memory_order_relaxed) || !g_events) return;
    const uint64_t sequence = g_sequence.fetch_add(1, std::memory_order_relaxed) + 1;
    if (sequence > g_cfg.maxEvents) {
        g_dropped.fetch_add(1, std::memory_order_relaxed);
        g_capture.store(false, std::memory_order_relaxed);
        return;
    }
    EnterCriticalSection(&g_logLock);
    fprintf(g_events, "%llu\t%llu\t%lu\t%s\t%p\t%p\t0x%llX\n",
        static_cast<unsigned long long>(sequence),
        static_cast<unsigned long long>(QpcMicroseconds()),
        GetCurrentThreadId(), eventName, self, context,
        static_cast<unsigned long long>(ToRva(caller)));
    LeaveCriticalSection(&g_logLock);
}

const char* StorageKindName(StorageKind kind) {
    switch (kind) {
    case StorageKind::Bool: return "bool";
    case StorageKind::Float: return "float";
    case StorageKind::U32A: return "u32_a";
    case StorageKind::U32B: return "u32_b";
    case StorageKind::RefA: return "ref_a";
    case StorageKind::RefB: return "ref_b";
    case StorageKind::RefC: return "ref_c";
    case StorageKind::U32C: return "u32_c";
    }
    return "unknown";
}

size_t StorageKindSize(StorageKind kind) {
    switch (kind) {
    case StorageKind::Bool: return sizeof(uint8_t);
    case StorageKind::RefA:
    case StorageKind::RefB:
    case StorageKind::RefC: return sizeof(uint64_t);
    default: return sizeof(uint32_t);
    }
}

void ResetConsumerTraceCaches() {
    AcquireSRWLockExclusive(&g_damageSlots.lock);
    g_damageSlots.count = 0;
    ReleaseSRWLockExclusive(&g_damageSlots.lock);
    AcquireSRWLockExclusive(&g_consumerSeen.lock);
    g_consumerSeen.count = 0;
    ReleaseSRWLockExclusive(&g_consumerSeen.lock);
    g_lastDamagePulseUs.store(0, std::memory_order_relaxed);
    g_consumerTraceUntilUs.store(0, std::memory_order_relaxed);
}

void RegisterDamageSlot(void* self, void* context, uint16_t slotOffset,
    uintptr_t getterRva, StorageKind kind) {
    if (!context) return;
    void* slot = nullptr;
    uint32_t tag = 0;
    __try {
        slot = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(context) + slotOffset);
        if (!slot) return;
        tag = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(slot) + 0x1C);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return;
    }

    AcquireSRWLockExclusive(&g_damageSlots.lock);
    for (size_t index = 0; index < g_damageSlots.count; ++index) {
        auto& entry = g_damageSlots.entries[index];
        if (entry.slot == slot && entry.slotOffset == slotOffset && entry.kind == kind) {
            entry.self = self;
            entry.context = context;
            entry.getterRva = getterRva;
            entry.tag = tag;
            ReleaseSRWLockExclusive(&g_damageSlots.lock);
            return;
        }
    }
    if (g_damageSlots.count < kMaxTrackedDamageSlots) {
        auto& entry = g_damageSlots.entries[g_damageSlots.count++];
        entry.slot = slot;
        entry.self = self;
        entry.context = context;
        entry.getterRva = getterRva;
        entry.tag = tag;
        entry.slotOffset = slotOffset;
        entry.kind = kind;
    }
    ReleaseSRWLockExclusive(&g_damageSlots.lock);
}

void RegisterDamageSlots(void* self, void* context) {
    if (!g_cfg.consumerTrace || !g_capture.load(std::memory_order_relaxed)) return;
    struct Descriptor {
        uint16_t offset;
        uintptr_t getterRva;
        StorageKind kind;
    };
    static const Descriptor descriptors[] = {
        {0x28, kRvaGetBoolStorage, StorageKind::Bool},
        {0x30, kRvaGetFloatStorage, StorageKind::Float},
        {0x38, kRvaGetBoolStorage, StorageKind::Bool},
        {0x40, kRvaGetFloatStorage, StorageKind::Float},
        {0x48, kRvaGetFloatStorage, StorageKind::Float},
        {0x50, kRvaGetDamageU32AStorage, StorageKind::U32A},
        {0x58, kRvaGetDamageU32BStorage, StorageKind::U32B},
        {0x60, kRvaGetDamageRefAStorage, StorageKind::RefA},
        {0x68, kRvaGetDamageRefBStorage, StorageKind::RefB},
        {0x70, kRvaGetFloatStorage, StorageKind::Float},
        {0x78, kRvaGetFloatStorage, StorageKind::Float},
        {0x80, kRvaGetDamageRefCStorage, StorageKind::RefC},
        {0x88, kRvaGetDamageU32CStorage, StorageKind::U32C},
        {0x90, kRvaGetBoolStorage, StorageKind::Bool},
    };
    for (const auto& descriptor : descriptors)
        RegisterDamageSlot(self, context, descriptor.offset, descriptor.getterRva, descriptor.kind);
}

bool LookupDamageSlot(void* slot, StorageKind kind, DamageSlotEntry* result) {
    bool found = false;
    AcquireSRWLockShared(&g_damageSlots.lock);
    for (size_t index = g_damageSlots.count; index > 0; --index) {
        const auto& entry = g_damageSlots.entries[index - 1];
        if (entry.slot == slot && entry.kind == kind) {
            if (result) *result = entry;
            found = true;
            break;
        }
    }
    ReleaseSRWLockShared(&g_damageSlots.lock);
    return found;
}

bool MarkConsumerSeen(uint64_t pulse, void* slot, uintptr_t callerRva) {
    bool first = false;
    AcquireSRWLockExclusive(&g_consumerSeen.lock);
    for (size_t index = 0; index < g_consumerSeen.count; ++index) {
        const auto& entry = g_consumerSeen.entries[index];
        if (entry.pulse == pulse && entry.slot == slot && entry.callerRva == callerRva) {
            ReleaseSRWLockExclusive(&g_consumerSeen.lock);
            return false;
        }
    }
    if (g_consumerSeen.count < kMaxConsumerSeen) {
        auto& entry = g_consumerSeen.entries[g_consumerSeen.count++];
        entry.pulse = pulse;
        entry.slot = slot;
        entry.callerRva = callerRva;
        first = true;
    }
    ReleaseSRWLockExclusive(&g_consumerSeen.lock);
    return first;
}

uint64_t ReadStorageBits(void* storage, StorageKind kind) {
    if (!storage) return 0;
    uint64_t bits = 0;
    __try {
        memcpy(&bits, storage, StorageKindSize(kind));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return 0;
    }
    return bits;
}

void TraceDamageConsumer(StorageKind kind, void* slot, void* storage, void* caller) {
    if (!g_cfg.consumerTrace || !g_consumers || g_insideDamageEval || g_observerStorageRead ||
        !g_capture.load(std::memory_order_relaxed)) return;
    const uint64_t nowUs = QpcMicroseconds();
    if (nowUs > g_consumerTraceUntilUs.load(std::memory_order_relaxed)) return;
    const uint64_t pulse = g_damagePulseNumber.load(std::memory_order_relaxed);
    if (!pulse) return;
    DamageSlotEntry entry = {};
    if (!LookupDamageSlot(slot, kind, &entry)) return;
    const uintptr_t callerRva = ToRva(caller);
    if (!callerRva || !MarkConsumerSeen(pulse, slot, callerRva)) return;

    const uint64_t sequence = g_consumerSequence.fetch_add(1, std::memory_order_relaxed) + 1;
    if (sequence > g_cfg.maxConsumerEvents) {
        g_consumerDropped.fetch_add(1, std::memory_order_relaxed);
        return;
    }
    void* stack[6] = {};
    const USHORT stackCount = CaptureStackBackTrace(2, 6, stack, nullptr);
    uintptr_t stackRvas[6] = {};
    for (USHORT index = 0; index < stackCount; ++index) stackRvas[index] = ToRva(stack[index]);
    const uint64_t valueBits = ReadStorageBits(storage, kind);

    EnterCriticalSection(&g_logLock);
    fprintf(g_consumers,
        "%llu\t%llu\t%lu\t%llu\t0x%02X\t%s\t0x%llX\t%p\t%p\t%p\t0x%08X\t0x%llX"
        "\t0x%llX\t0x%llX\t0x%llX\t0x%llX\t0x%llX\t0x%llX\n",
        static_cast<unsigned long long>(sequence), static_cast<unsigned long long>(nowUs),
        GetCurrentThreadId(), static_cast<unsigned long long>(pulse), entry.slotOffset,
        StorageKindName(kind), static_cast<unsigned long long>(valueBits), slot,
        entry.self, entry.context, entry.tag, static_cast<unsigned long long>(callerRva),
        static_cast<unsigned long long>(stackRvas[0]),
        static_cast<unsigned long long>(stackRvas[1]),
        static_cast<unsigned long long>(stackRvas[2]),
        static_cast<unsigned long long>(stackRvas[3]),
        static_cast<unsigned long long>(stackRvas[4]),
        static_cast<unsigned long long>(stackRvas[5]));
    LeaveCriticalSection(&g_logLock);
}

void ArmDamageConsumerTrace() {
    if (!g_cfg.consumerTrace || !g_capture.load(std::memory_order_relaxed)) return;
    const uint64_t nowUs = QpcMicroseconds();
    uint64_t previousUs = g_lastDamagePulseUs.load(std::memory_order_relaxed);
    while (nowUs > previousUs + 5000) {
        if (g_lastDamagePulseUs.compare_exchange_weak(previousUs, nowUs,
                std::memory_order_relaxed, std::memory_order_relaxed)) {
            const uint64_t pulse = g_damagePulseNumber.fetch_add(1, std::memory_order_relaxed) + 1;
            AcquireSRWLockExclusive(&g_consumerSeen.lock);
            g_consumerSeen.count = 0;
            ReleaseSRWLockExclusive(&g_consumerSeen.lock);
            g_consumerTraceUntilUs.store(
                nowUs + static_cast<uint64_t>(g_cfg.consumerTraceWindowMs) * 1000,
                std::memory_order_relaxed);
            StateTrace("DamageConsumerTrace.armed", nullptr, nullptr,
                "pulse=%llu;window_ms=%lu", static_cast<unsigned long long>(pulse),
                g_cfg.consumerTraceWindowMs);
            return;
        }
    }
}

void* __fastcall HookGetBoolStorage(void* slot, uint32_t index) {
    void* caller = _ReturnAddress();
    void* storage = g_origGetBoolStorage(slot, index);
    TraceDamageConsumer(StorageKind::Bool, slot, storage, caller);
    return storage;
}

void* __fastcall HookGetFloatStorage(void* slot, uint32_t index) {
    void* caller = _ReturnAddress();
    void* storage = g_origGetFloatStorage(slot, index);
    TraceDamageConsumer(StorageKind::Float, slot, storage, caller);
    return storage;
}

#define DEFINE_STORAGE_HOOK(functionName, originalName, kindName) \
    void* __fastcall functionName(void* slot, uint32_t index) { \
        void* caller = _ReturnAddress(); \
        void* storage = originalName(slot, index); \
        TraceDamageConsumer(kindName, slot, storage, caller); \
        return storage; \
    }

DEFINE_STORAGE_HOOK(HookGetDamageU32AStorage, g_origGetDamageU32AStorage, StorageKind::U32A)
DEFINE_STORAGE_HOOK(HookGetDamageU32BStorage, g_origGetDamageU32BStorage, StorageKind::U32B)
DEFINE_STORAGE_HOOK(HookGetDamageRefAStorage, g_origGetDamageRefAStorage, StorageKind::RefA)
DEFINE_STORAGE_HOOK(HookGetDamageRefBStorage, g_origGetDamageRefBStorage, StorageKind::RefB)
DEFINE_STORAGE_HOOK(HookGetDamageRefCStorage, g_origGetDamageRefCStorage, StorageKind::RefC)
DEFINE_STORAGE_HOOK(HookGetDamageU32CStorage, g_origGetDamageU32CStorage, StorageKind::U32C)

#undef DEFINE_STORAGE_HOOK

bool ReadSlotValue(void* context, size_t slotOffset, uintptr_t getterRva,
    void* output, size_t outputSize) {
    if (!context || !output || !outputSize) return false;
    __try {
        void* slot = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(context) + slotOffset);
        if (!slot) return false;
        const uint32_t tag = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(slot) + 0x1C);
        void* storage = slot;
        if ((tag & 0x1000FFFFu) != 1u) {
            if (!getterRva || getterRva >= g_imageSize) return false;
            auto getter = reinterpret_cast<SlotGetterFn>(g_base + getterRva);
            __try {
                g_observerStorageRead = true;
                storage = getter(slot, 1);
            } __finally {
                g_observerStorageRead = false;
            }
        }
        if (!storage) return false;
        memcpy(output, storage, outputSize);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool WriteSlotValue(void* context, size_t slotOffset, uintptr_t getterRva,
    const void* input, size_t inputSize) {
    if (!context || !input || !inputSize) return false;
    __try {
        void* slot = *reinterpret_cast<void**>(reinterpret_cast<uint8_t*>(context) + slotOffset);
        if (!slot) return false;
        const uint32_t tag = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(slot) + 0x1C);
        void* storage = slot;
        if ((tag & 0x1000FFFFu) != 1u) {
            if (!getterRva || getterRva >= g_imageSize) return false;
            auto getter = reinterpret_cast<SlotGetterFn>(g_base + getterRva);
            __try {
                g_observerStorageRead = true;
                storage = getter(slot, 1);
            } __finally {
                g_observerStorageRead = false;
            }
        }
        if (!storage) return false;
        memcpy(storage, input, inputSize);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ReadDirectValue(const void* address, void* output, size_t outputSize) {
    if (!address || !output || !outputSize) return false;
    __try {
        memcpy(output, address, outputSize);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

const char* MatchTrailblazerCouchUid(const uint8_t uid[16]) {
    // Canonical asset UID: 928CE2A16537C3B40000DEDF5968E5FD. Snowdrop UID
    // byte ordering is not yet established for this component, so accept the
    // common direct, GUID, word, qword, and fully reversed layouts.
    static const uint8_t direct[] = {
        0x92,0x8C,0xE2,0xA1,0x65,0x37,0xC3,0xB4,
        0x00,0x00,0xDE,0xDF,0x59,0x68,0xE5,0xFD,
    };
    static const uint8_t guid[] = {
        0xA1,0xE2,0x8C,0x92,0x37,0x65,0xB4,0xC3,
        0x00,0x00,0xDE,0xDF,0x59,0x68,0xE5,0xFD,
    };
    static const uint8_t words[] = {
        0xA1,0xE2,0x8C,0x92,0xB4,0xC3,0x37,0x65,
        0xDF,0xDE,0x00,0x00,0xFD,0xE5,0x68,0x59,
    };
    static const uint8_t qwords[] = {
        0xB4,0xC3,0x37,0x65,0xA1,0xE2,0x8C,0x92,
        0xFD,0xE5,0x68,0x59,0xDF,0xDE,0x00,0x00,
    };
    static const uint8_t reversed[] = {
        0xFD,0xE5,0x68,0x59,0xDF,0xDE,0x00,0x00,
        0xB4,0xC3,0x37,0x65,0xA1,0xE2,0x8C,0x92,
    };
    if (memcmp(uid, direct, sizeof(direct)) == 0) return "direct";
    if (memcmp(uid, guid, sizeof(guid)) == 0) return "guid";
    if (memcmp(uid, words, sizeof(words)) == 0) return "u32_le";
    if (memcmp(uid, qwords, sizeof(qwords)) == 0) return "u64_le";
    if (memcmp(uid, reversed, sizeof(reversed)) == 0) return "reversed";
    return nullptr;
}

void ReadCouchThreadMarker(uint64_t* generation, void** component) {
    const DWORD threadId = GetCurrentThreadId();
    uint64_t foundGeneration = 0;
    void* foundComponent = nullptr;
    AcquireSRWLockShared(&g_couchThreadMarkers.lock);
    for (size_t index = 0; index < g_couchThreadMarkers.count; ++index) {
        const CouchThreadMarker& entry = g_couchThreadMarkers.entries[index];
        if (entry.threadId != threadId) continue;
        foundGeneration = entry.generation;
        foundComponent = entry.component;
        break;
    }
    ReleaseSRWLockShared(&g_couchThreadMarkers.lock);
    if (generation) *generation = foundGeneration;
    if (component) *component = foundComponent;
}

void MarkCouchFromArchive(void* component) {
    const DWORD threadId = GetCurrentThreadId();
    AcquireSRWLockExclusive(&g_couchThreadMarkers.lock);
    size_t index = g_couchThreadMarkers.count;
    for (size_t candidate = 0; candidate < g_couchThreadMarkers.count; ++candidate) {
        if (g_couchThreadMarkers.entries[candidate].threadId == threadId) {
            index = candidate;
            break;
        }
    }
    if (index == g_couchThreadMarkers.count) {
        if (g_couchThreadMarkers.count < kMaxCouchThreadMarkers) {
            ++g_couchThreadMarkers.count;
        } else {
            index = threadId % kMaxCouchThreadMarkers;
            g_couchThreadMarkers.entries[index] = {};
            g_couchThreadMarkers.entries[index].threadId = threadId;
        }
    }
    CouchThreadMarker& entry = g_couchThreadMarkers.entries[index];
    entry.threadId = threadId;
    ++entry.generation;
    entry.component = component;
    ReleaseSRWLockExclusive(&g_couchThreadMarkers.lock);
}

uint64_t RememberCouchComponent(void* component) {
    if (!component) return 0;
    void* owner = nullptr;
    ReadDirectValue(static_cast<uint8_t*>(component) + 0x10, &owner, sizeof(owner));
    AcquireSRWLockExclusive(&g_couchComponents.lock);
    const uint64_t sequence = ++g_couchComponents.sequence;
    size_t index = g_couchComponents.count;
    for (size_t candidate = 0; candidate < g_couchComponents.count; ++candidate) {
        if (g_couchComponents.entries[candidate].component == component) {
            index = candidate;
            break;
        }
    }
    if (index == g_couchComponents.count) {
        if (g_couchComponents.count < kMaxCouchComponents) {
            ++g_couchComponents.count;
        } else {
            index = 0;
            for (size_t candidate = 1; candidate < g_couchComponents.count; ++candidate) {
                if (g_couchComponents.entries[candidate].seenSequence <
                    g_couchComponents.entries[index].seenSequence) index = candidate;
            }
        }
    }
    CouchComponentEntry& entry = g_couchComponents.entries[index];
    entry.component = component;
    entry.seenSequence = sequence;
    entry.ownerPresent = owner != nullptr;
    ReleaseSRWLockExclusive(&g_couchComponents.lock);
    g_uidWatchCandidateSequence.store(sequence, std::memory_order_release);
    return sequence;
}

void WaitForUidWatchSequence(uint64_t sequence) {
    if (!sequence || !g_uidWatchRequested.load(std::memory_order_acquire)) return;
    const ULONGLONG deadline = GetTickCount64() + 500;
    while (g_running.load(std::memory_order_relaxed) &&
        g_uidWatchRequested.load(std::memory_order_acquire) &&
        GetTickCount64() < deadline) {
        if (g_uidWatchArmed.load(std::memory_order_acquire) &&
            g_uidWatchArmedCandidateSequence.load(std::memory_order_acquire) >= sequence) {
            return;
        }
        Sleep(1);
    }
    if (!g_uidWatchRequested.load(std::memory_order_acquire)) return;
    Log("couch UID watch synchronization timeout candidate_sequence=%llu armed_sequence=%llu",
        static_cast<unsigned long long>(sequence),
        static_cast<unsigned long long>(
            g_uidWatchArmedCandidateSequence.load(std::memory_order_relaxed)));
}

bool IsCurrentCouchComponent(void* component) {
    uint8_t uid[16] = {};
    return component && ReadDirectValue(static_cast<uint8_t*>(component) + 0x18,
        uid, sizeof(uid)) && MatchTrailblazerCouchUid(uid) != nullptr;
}

size_t SelectUidWatchTargets(UidWatchTarget targets[kMaxUidWatchTargets],
    uint64_t* selectedSequence) {
    CouchComponentEntry entries[kMaxCouchComponents] = {};
    size_t count = 0;
    uint64_t sequence = 0;
    AcquireSRWLockShared(&g_couchComponents.lock);
    count = g_couchComponents.count;
    if (count > kMaxCouchComponents) count = kMaxCouchComponents;
    memcpy(entries, g_couchComponents.entries, count * sizeof(entries[0]));
    sequence = g_couchComponents.sequence;
    ReleaseSRWLockShared(&g_couchComponents.lock);

    size_t selected = 0;
    bool used[kMaxCouchComponents] = {};
    while (selected + 1 < kMaxUidWatchTargets) {
        size_t newest = count;
        for (size_t index = 0; index < count; ++index) {
            if (used[index]) continue;
            if (newest == count ||
                entries[index].seenSequence > entries[newest].seenSequence) newest = index;
        }
        if (newest == count) break;
        used[newest] = true;
        if (!IsCurrentCouchComponent(entries[newest].component)) continue;
        targets[selected].component = entries[newest].component;
        targets[selected].address = reinterpret_cast<uintptr_t>(
            static_cast<uint8_t*>(entries[newest].component) + 0x18);
        ++selected;
        targets[selected].component = entries[newest].component;
        targets[selected].address = reinterpret_cast<uintptr_t>(
            static_cast<uint8_t*>(entries[newest].component) + 0x20);
        ++selected;
    }
    if (selectedSequence) *selectedSequence = sequence;
    return selected;
}

LONG CALLBACK UidWatchExceptionHandler(EXCEPTION_POINTERS* exception) {
    if (!exception || !exception->ExceptionRecord || !exception->ContextRecord ||
        exception->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP ||
        !g_uidWatchArmed.load(std::memory_order_acquire)) {
        return EXCEPTION_CONTINUE_SEARCH;
    }
    CONTEXT* context = exception->ContextRecord;
    // All four local debug slots belong to this diagnostic on participating
    // threads. Keep accepting every slot while targets are being rotated so a
    // thread still carrying the previous set cannot leak a single-step fault.
    const DWORD64 triggered = context->Dr6 & 0x0FULL;
    if (!triggered) return EXCEPTION_CONTINUE_SEARCH;
    context->Dr6 &= ~0x0FULL;
    if (g_uidWatchStopping.load(std::memory_order_relaxed))
        return EXCEPTION_CONTINUE_EXECUTION;

    uint32_t slot = 0;
    while (slot < g_uidWatchTargetCount && (triggered & (1ULL << slot)) == 0) ++slot;
    if (slot >= g_uidWatchTargetCount) return EXCEPTION_CONTINUE_EXECUTION;
    const uintptr_t nextRip = static_cast<uintptr_t>(context->Rip);
    if (g_observerBase && nextRip >= g_observerBase &&
        nextRip < g_observerBase + g_observerSize) {
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (!IsCurrentCouchComponent(g_uidWatchTargets[slot].component)) {
        g_uidWatchStaleIgnored.fetch_add(1, std::memory_order_relaxed);
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    const uint32_t index = g_uidWatchWriteIndex.fetch_add(1, std::memory_order_relaxed);
    if (index >= kMaxUidWatchHits) {
        g_uidWatchDropped.fetch_add(1, std::memory_order_relaxed);
        g_uidWatchStopRequested.store(true, std::memory_order_release);
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    UidWatchHit& hit = g_uidWatchHits[index];
    hit.ready.store(0, std::memory_order_relaxed);
    hit.threadId = GetCurrentThreadId();
    hit.slot = slot;
    LARGE_INTEGER now = {};
    QueryPerformanceCounter(&now);
    hit.qpc = static_cast<uint64_t>(now.QuadPart);
    hit.component = reinterpret_cast<uintptr_t>(g_uidWatchTargets[slot].component);
    hit.address = g_uidWatchTargets[slot].address;
    hit.nextRip = nextRip;
    hit.rsp = static_cast<uintptr_t>(context->Rsp);
    hit.rbp = static_cast<uintptr_t>(context->Rbp);
    hit.rax = static_cast<uintptr_t>(context->Rax);
    hit.rbx = static_cast<uintptr_t>(context->Rbx);
    hit.rcx = static_cast<uintptr_t>(context->Rcx);
    hit.rdx = static_cast<uintptr_t>(context->Rdx);
    hit.rsi = static_cast<uintptr_t>(context->Rsi);
    hit.rdi = static_cast<uintptr_t>(context->Rdi);
    hit.r8 = static_cast<uintptr_t>(context->R8);
    hit.r9 = static_cast<uintptr_t>(context->R9);
    hit.r10 = static_cast<uintptr_t>(context->R10);
    hit.r11 = static_cast<uintptr_t>(context->R11);
    hit.r12 = static_cast<uintptr_t>(context->R12);
    hit.r13 = static_cast<uintptr_t>(context->R13);
    hit.r14 = static_cast<uintptr_t>(context->R14);
    hit.r15 = static_cast<uintptr_t>(context->R15);
    __try {
        memcpy(hit.stackWords, reinterpret_cast<const void*>(hit.rsp),
            sizeof(hit.stackWords));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        memset(hit.stackWords, 0, sizeof(hit.stackWords));
    }
    __try {
        memcpy(hit.codeBefore, reinterpret_cast<const void*>(nextRip - sizeof(hit.codeBefore)),
            sizeof(hit.codeBefore));
        memcpy(hit.codeAfter, reinterpret_cast<const void*>(nextRip), sizeof(hit.codeAfter));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        memset(hit.codeBefore, 0, sizeof(hit.codeBefore));
        memset(hit.codeAfter, 0, sizeof(hit.codeAfter));
    }
    hit.ready.store(1, std::memory_order_release);
    if (index + 1 >= kMaxUidWatchHits)
        g_uidWatchStopRequested.store(true, std::memory_order_release);
    return EXCEPTION_CONTINUE_EXECUTION;
}

void RestoreUidWatchThreads() {
    g_uidWatchStopping.store(true, std::memory_order_release);
    size_t restored = 0;
    for (size_t index = 0; index < g_uidWatchThreadCount; ++index) {
        const UidWatchThreadState& saved = g_uidWatchThreads[index];
        HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
            THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, saved.threadId);
        if (!thread) continue;
        if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
            CloseHandle(thread);
            continue;
        }
        CONTEXT context = {};
        context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(thread, &context)) {
            context.Dr0 = saved.dr0;
            context.Dr1 = saved.dr1;
            context.Dr2 = saved.dr2;
            context.Dr3 = saved.dr3;
            context.Dr6 = saved.dr6;
            context.Dr7 = saved.dr7;
            if (SetThreadContext(thread, &context)) ++restored;
        }
        ResumeThread(thread);
        CloseHandle(thread);
    }
    g_uidWatchThreadCount = 0;
    g_uidWatchArmed.store(false, std::memory_order_release);
    g_uidWatchStopping.store(false, std::memory_order_release);
    Log("couch UID watch disarmed restored_threads=%llu hits=%u dropped=%u stale_ignored=%u",
        static_cast<unsigned long long>(restored),
        g_uidWatchWriteIndex.load(std::memory_order_relaxed),
        g_uidWatchDropped.load(std::memory_order_relaxed),
        g_uidWatchStaleIgnored.load(std::memory_order_relaxed));
}

bool ArmUidWatch() {
    if (!g_cfg.couchUidWatch || g_uidWatchArmed.load(std::memory_order_acquire)) return false;
    UidWatchTarget targets[kMaxUidWatchTargets] = {};
    uint64_t candidateSequence = 0;
    const size_t targetCount = SelectUidWatchTargets(targets, &candidateSequence);
    if (!targetCount) return false;
    if (!g_uidWatchVeh) {
        g_uidWatchVeh = AddVectoredExceptionHandler(1, &UidWatchExceptionHandler);
        if (!g_uidWatchVeh) {
            Log("couch UID watch could not install vectored exception handler error=%lu",
                GetLastError());
            return false;
        }
    }
    memcpy(g_uidWatchTargets, targets, sizeof(g_uidWatchTargets));
    g_uidWatchTargetCount = targetCount;
    g_uidWatchThreadCount = 0;
    g_uidWatchDrainIndex = 0;
    g_uidWatchWriteIndex.store(0, std::memory_order_relaxed);
    g_uidWatchDropped.store(0, std::memory_order_relaxed);
    g_uidWatchStaleIgnored.store(0, std::memory_order_relaxed);
    g_uidWatchStopRequested.store(false, std::memory_order_relaxed);
    for (size_t index = 0; index < kMaxUidWatchHits; ++index)
        g_uidWatchHits[index].ready.store(0, std::memory_order_relaxed);
    g_uidWatchArmed.store(true, std::memory_order_release);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    size_t skippedExisting = 0;
    size_t failed = 0;
    if (snapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 entry = {};
        entry.dwSize = sizeof(entry);
        const DWORD processId = GetCurrentProcessId();
        const DWORD workerThreadId = GetCurrentThreadId();
        if (Thread32First(snapshot, &entry)) {
            do {
                if (entry.th32OwnerProcessID != processId ||
                    entry.th32ThreadID == workerThreadId ||
                    g_uidWatchThreadCount >= kMaxUidWatchThreads) continue;
                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                    THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, entry.th32ThreadID);
                if (!thread) {
                    ++failed;
                    continue;
                }
                if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
                    ++failed;
                    CloseHandle(thread);
                    continue;
                }
                CONTEXT context = {};
                context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                if (!GetThreadContext(thread, &context)) {
                    ++failed;
                    ResumeThread(thread);
                    CloseHandle(thread);
                    continue;
                }
                if ((context.Dr7 & 0xFFULL) != 0) {
                    ++skippedExisting;
                    ResumeThread(thread);
                    CloseHandle(thread);
                    continue;
                }
                UidWatchThreadState& saved = g_uidWatchThreads[g_uidWatchThreadCount];
                saved.threadId = entry.th32ThreadID;
                saved.dr0 = context.Dr0;
                saved.dr1 = context.Dr1;
                saved.dr2 = context.Dr2;
                saved.dr3 = context.Dr3;
                saved.dr6 = context.Dr6;
                saved.dr7 = context.Dr7;
                context.Dr0 = targetCount > 0 ? targets[0].address : 0;
                context.Dr1 = targetCount > 1 ? targets[1].address : 0;
                context.Dr2 = targetCount > 2 ? targets[2].address : 0;
                context.Dr3 = targetCount > 3 ? targets[3].address : 0;
                context.Dr6 = 0;
                context.Dr7 &= ~((DWORD64)0xFFULL | ((DWORD64)0xFFFFULL << 16));
                for (size_t slot = 0; slot < targetCount; ++slot) {
                    context.Dr7 |= 1ULL << (slot * 2);
                    context.Dr7 |= 0xBULL << (16 + slot * 4);
                }
                if (SetThreadContext(thread, &context)) {
                    ++g_uidWatchThreadCount;
                } else {
                    ++failed;
                }
                ResumeThread(thread);
                CloseHandle(thread);
            } while (Thread32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }
    if (!g_uidWatchThreadCount) {
        g_uidWatchArmed.store(false, std::memory_order_release);
        g_uidWatchTargetCount = 0;
        Log("couch UID watch found targets but armed no threads failed=%llu existing=%llu",
            static_cast<unsigned long long>(failed),
            static_cast<unsigned long long>(skippedExisting));
        return false;
    }
    g_uidWatchArmedCandidateSequence.store(candidateSequence, std::memory_order_release);
    char targetText[512] = {};
    int used = 0;
    for (size_t slot = 0; slot < targetCount && used >= 0 &&
        static_cast<size_t>(used) < sizeof(targetText); ++slot) {
        used += snprintf(targetText + used, sizeof(targetText) - static_cast<size_t>(used),
            "%s%llu:%p", slot ? "," : "", static_cast<unsigned long long>(slot),
            reinterpret_cast<void*>(targets[slot].address));
    }
    Log("couch UID watch armed targets=%llu threads=%llu skipped_existing=%llu failed=%llu "
        "addresses=%s",
        static_cast<unsigned long long>(targetCount),
        static_cast<unsigned long long>(g_uidWatchThreadCount),
        static_cast<unsigned long long>(skippedExisting),
        static_cast<unsigned long long>(failed), targetText);
    return true;
}

bool RefreshUidWatchTargets() {
    if (!g_uidWatchArmed.load(std::memory_order_acquire)) return false;

    UidWatchTarget targets[kMaxUidWatchTargets] = {};
    uint64_t candidateSequence = 0;
    const size_t targetCount = SelectUidWatchTargets(targets, &candidateSequence);
    if (!targetCount) return false;

    bool changed = targetCount != g_uidWatchTargetCount;
    for (size_t slot = 0; !changed && slot < targetCount; ++slot) {
        changed = targets[slot].address != g_uidWatchTargets[slot].address;
    }
    if (!changed) {
        g_uidWatchArmedCandidateSequence.store(candidateSequence, std::memory_order_release);
        return false;
    }

    g_uidWatchStopping.store(true, std::memory_order_release);
    size_t updated = 0;
    size_t added = 0;
    size_t skippedExisting = 0;
    size_t failed = 0;
    for (size_t index = 0; index < g_uidWatchThreadCount; ++index) {
        const UidWatchThreadState& saved = g_uidWatchThreads[index];
        HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
            THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, saved.threadId);
        if (!thread) {
            ++failed;
            continue;
        }
        if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
            ++failed;
            CloseHandle(thread);
            continue;
        }
        CONTEXT context = {};
        context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
        if (GetThreadContext(thread, &context)) {
            context.Dr0 = targetCount > 0 ? targets[0].address : 0;
            context.Dr1 = targetCount > 1 ? targets[1].address : 0;
            context.Dr2 = targetCount > 2 ? targets[2].address : 0;
            context.Dr3 = targetCount > 3 ? targets[3].address : 0;
            context.Dr6 = 0;
            context.Dr7 &= ~((DWORD64)0xFFULL | ((DWORD64)0xFFFFULL << 16));
            for (size_t slot = 0; slot < targetCount; ++slot) {
                context.Dr7 |= 1ULL << (slot * 2);
                context.Dr7 |= 0xBULL << (16 + slot * 4);
            }
            if (SetThreadContext(thread, &context)) {
                ++updated;
            } else {
                ++failed;
            }
        } else {
            ++failed;
        }
        ResumeThread(thread);
        CloseHandle(thread);
    }

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snapshot != INVALID_HANDLE_VALUE) {
        THREADENTRY32 entry = {};
        entry.dwSize = sizeof(entry);
        const DWORD processId = GetCurrentProcessId();
        const DWORD workerThreadId = GetCurrentThreadId();
        if (Thread32First(snapshot, &entry)) {
            do {
                if (entry.th32OwnerProcessID != processId ||
                    entry.th32ThreadID == workerThreadId ||
                    g_uidWatchThreadCount >= kMaxUidWatchThreads) continue;
                bool known = false;
                for (size_t index = 0; index < g_uidWatchThreadCount; ++index) {
                    if (g_uidWatchThreads[index].threadId == entry.th32ThreadID) {
                        known = true;
                        break;
                    }
                }
                if (known) continue;

                HANDLE thread = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT |
                    THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION,
                    FALSE, entry.th32ThreadID);
                if (!thread) {
                    ++failed;
                    continue;
                }
                if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
                    ++failed;
                    CloseHandle(thread);
                    continue;
                }
                CONTEXT context = {};
                context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
                if (!GetThreadContext(thread, &context)) {
                    ++failed;
                    ResumeThread(thread);
                    CloseHandle(thread);
                    continue;
                }
                if ((context.Dr7 & 0xFFULL) != 0) {
                    ++skippedExisting;
                    ResumeThread(thread);
                    CloseHandle(thread);
                    continue;
                }
                UidWatchThreadState& saved = g_uidWatchThreads[g_uidWatchThreadCount];
                saved.threadId = entry.th32ThreadID;
                saved.dr0 = context.Dr0;
                saved.dr1 = context.Dr1;
                saved.dr2 = context.Dr2;
                saved.dr3 = context.Dr3;
                saved.dr6 = context.Dr6;
                saved.dr7 = context.Dr7;
                context.Dr0 = targetCount > 0 ? targets[0].address : 0;
                context.Dr1 = targetCount > 1 ? targets[1].address : 0;
                context.Dr2 = targetCount > 2 ? targets[2].address : 0;
                context.Dr3 = targetCount > 3 ? targets[3].address : 0;
                context.Dr6 = 0;
                context.Dr7 &= ~((DWORD64)0xFFULL | ((DWORD64)0xFFFFULL << 16));
                for (size_t slot = 0; slot < targetCount; ++slot) {
                    context.Dr7 |= 1ULL << (slot * 2);
                    context.Dr7 |= 0xBULL << (16 + slot * 4);
                }
                if (SetThreadContext(thread, &context)) {
                    ++g_uidWatchThreadCount;
                    ++added;
                } else {
                    ++failed;
                }
                ResumeThread(thread);
                CloseHandle(thread);
            } while (Thread32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);
    }

    memcpy(g_uidWatchTargets, targets, sizeof(g_uidWatchTargets));
    g_uidWatchTargetCount = targetCount;
    g_uidWatchStopping.store(false, std::memory_order_release);
    g_uidWatchArmedCandidateSequence.store(candidateSequence, std::memory_order_release);

    char targetText[512] = {};
    int used = 0;
    for (size_t slot = 0; slot < targetCount && used >= 0 &&
        static_cast<size_t>(used) < sizeof(targetText); ++slot) {
        used += snprintf(targetText + used, sizeof(targetText) - static_cast<size_t>(used),
            "%s%llu:%p", slot ? "," : "", static_cast<unsigned long long>(slot),
            reinterpret_cast<void*>(targets[slot].address));
    }
    Log("couch UID watch targets refreshed targets=%llu updated_threads=%llu "
        "added_threads=%llu skipped_existing=%llu failed=%llu "
        "candidate_sequence=%llu addresses=%s",
        static_cast<unsigned long long>(targetCount),
        static_cast<unsigned long long>(updated),
        static_cast<unsigned long long>(added),
        static_cast<unsigned long long>(skippedExisting),
        static_cast<unsigned long long>(failed),
        static_cast<unsigned long long>(candidateSequence), targetText);
    return true;
}

void DrainUidWatchHits() {
    const uint32_t available = g_uidWatchWriteIndex.load(std::memory_order_acquire);
    const uint32_t limit = available < kMaxUidWatchHits ? available : kMaxUidWatchHits;
    while (g_uidWatchDrainIndex < limit) {
        UidWatchHit& hit = g_uidWatchHits[g_uidWatchDrainIndex];
        if (!hit.ready.load(std::memory_order_acquire)) break;
        char stackText[512] = {};
        int used = 0;
        for (size_t index = 0; index < kUidWatchStackWords && used >= 0 &&
            static_cast<size_t>(used) < sizeof(stackText); ++index) {
            used += snprintf(stackText + used, sizeof(stackText) - static_cast<size_t>(used),
                "%s%p", index ? "," : "",
                reinterpret_cast<void*>(hit.stackWords[index]));
        }
        char beforeText[33] = {};
        char afterText[33] = {};
        for (size_t index = 0; index < 16; ++index) {
            snprintf(beforeText + index * 2, sizeof(beforeText) - index * 2,
                "%02X", hit.codeBefore[index]);
            snprintf(afterText + index * 2, sizeof(afterText) - index * 2,
                "%02X", hit.codeAfter[index]);
        }
        uint64_t elapsedUs = 0;
        if (g_qpcFrequency.QuadPart > 0 && hit.qpc >= static_cast<uint64_t>(g_qpcStart.QuadPart)) {
            elapsedUs = (hit.qpc - static_cast<uint64_t>(g_qpcStart.QuadPart)) * 1000000ULL /
                static_cast<uint64_t>(g_qpcFrequency.QuadPart);
        }
        Log("couch UID watch hit=%u us=%llu tid=%lu slot=%u component=%p address=%p "
            "next_rip=%p next_rip_rva=0x%llX rsp=%p rbp=%p "
            "rax=%p rbx=%p rcx=%p rdx=%p rsi=%p rdi=%p r8=%p r9=%p r10=%p "
            "r11=%p r12=%p r13=%p r14=%p r15=%p code_before=%s code_after=%s "
            "stack_words=%s",
            g_uidWatchDrainIndex + 1, static_cast<unsigned long long>(elapsedUs),
            hit.threadId, hit.slot, reinterpret_cast<void*>(hit.component),
            reinterpret_cast<void*>(hit.address), reinterpret_cast<void*>(hit.nextRip),
            static_cast<unsigned long long>(ToRva(reinterpret_cast<void*>(hit.nextRip))),
            reinterpret_cast<void*>(hit.rsp), reinterpret_cast<void*>(hit.rbp),
            reinterpret_cast<void*>(hit.rax), reinterpret_cast<void*>(hit.rbx),
            reinterpret_cast<void*>(hit.rcx), reinterpret_cast<void*>(hit.rdx),
            reinterpret_cast<void*>(hit.rsi), reinterpret_cast<void*>(hit.rdi),
            reinterpret_cast<void*>(hit.r8), reinterpret_cast<void*>(hit.r9),
            reinterpret_cast<void*>(hit.r10), reinterpret_cast<void*>(hit.r11),
            reinterpret_cast<void*>(hit.r12), reinterpret_cast<void*>(hit.r13),
            reinterpret_cast<void*>(hit.r14), reinterpret_cast<void*>(hit.r15),
            beforeText, afterText, stackText);
        ++g_uidWatchDrainIndex;
    }
}

void ToggleUidWatch() {
    if (!g_cfg.couchUidWatch) {
        ShowOverlay("COUCH UID WATCH DISABLED IN INI");
        return;
    }
    const bool requested = g_uidWatchRequested.load(std::memory_order_relaxed);
    if (requested) {
        g_uidWatchRequested.store(false, std::memory_order_release);
        if (g_uidWatchArmed.load(std::memory_order_acquire)) RestoreUidWatchThreads();
        Log("couch UID watch request OFF by F5");
        ShowOverlay("COUCH UID WATCH OFF");
        return;
    }
    g_uidWatchRequested.store(true, std::memory_order_release);
    Log("couch UID watch request ON by F5");
    if (ArmUidWatch()) {
        ShowOverlay("COUCH UID WATCH ARMED");
    } else {
        ShowOverlay("COUCH UID WATCH WAITING FOR LOAD", 2200);
    }
}

void DumpCouchSequenceOwner(uint64_t matchNumber, void* component) {
    void* owner = nullptr;
    if (!component || !ReadDirectValue(static_cast<uint8_t*>(component) + 0x10,
            &owner, sizeof(owner)) || !owner) return;
    uintptr_t words[0x100 / sizeof(uintptr_t)] = {};
    if (!ReadDirectValue(owner, words, sizeof(words))) {
        Log("Trailblazer couch owner match=%llu component=%p owner=%p unreadable",
            static_cast<unsigned long long>(matchNumber), component, owner);
        return;
    }
    char line[3072] = {};
    int used = snprintf(line, sizeof(line),
        "Trailblazer couch owner match=%llu component=%p owner=%p "
        "vtable=%p vtable_rva=0x%llX words",
        static_cast<unsigned long long>(matchNumber), component, owner,
        reinterpret_cast<void*>(words[0]),
        static_cast<unsigned long long>(ToRva(reinterpret_cast<void*>(words[0]))));
    for (size_t index = 0; index < sizeof(words) / sizeof(words[0]) && used >= 0 &&
        static_cast<size_t>(used) < sizeof(line); ++index) {
        used += snprintf(line + used, sizeof(line) - static_cast<size_t>(used),
            " +%02llX=%p", static_cast<unsigned long long>(index * sizeof(uintptr_t)),
            reinterpret_cast<void*>(words[index]));
    }
    Log("%s", line);
}

void CaptureSequenceLevelEntityUid(const char* phase, void* component, void* archive,
    void* caller) {
    uint8_t uid[16] = {};
    if (!component || !ReadDirectValue(static_cast<uint8_t*>(component) + 0x18,
            uid, sizeof(uid))) return;
    const char* layout = MatchTrailblazerCouchUid(uid);
    if (layout) {
        if (strcmp(phase, "from_archive") == 0) MarkCouchFromArchive(component);
        const uint64_t candidateSequence = RememberCouchComponent(component);
        WaitForUidWatchSequence(candidateSequence);
        const uint64_t matchNumber =
            g_sequenceLevelEntityCouchMatches.fetch_add(1, std::memory_order_relaxed) + 1;
        uint8_t componentBytes[0x28] = {};
        const bool componentReadable = ReadDirectValue(
            component, componentBytes, sizeof(componentBytes));
        void* stack[12] = {};
        const USHORT stackCount = CaptureStackBackTrace(2, 12, stack, nullptr);
        char stackText[384] = {};
        int used = 0;
        for (USHORT index = 0; index < stackCount && used >= 0 &&
            static_cast<size_t>(used) < sizeof(stackText); ++index) {
            used += snprintf(stackText + used,
                sizeof(stackText) - static_cast<size_t>(used), "%s0x%llX",
                index ? "," : "",
                static_cast<unsigned long long>(ToRva(stack[index])));
        }
        Log("Trailblazer couch level entity match=%llu phase=%s component=%p "
            "archive=%p caller_rva=0x%llX uid_layout=%s component_readable=%s "
            "component_bytes="
            "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
            "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X"
            "%02X%02X%02X%02X%02X%02X%02X%02X stack_rvas=%s",
            static_cast<unsigned long long>(matchNumber), phase, component, archive,
            static_cast<unsigned long long>(ToRva(caller)), layout,
            componentReadable ? "YES" : "NO",
            componentBytes[0], componentBytes[1], componentBytes[2], componentBytes[3],
            componentBytes[4], componentBytes[5], componentBytes[6], componentBytes[7],
            componentBytes[8], componentBytes[9], componentBytes[10], componentBytes[11],
            componentBytes[12], componentBytes[13], componentBytes[14], componentBytes[15],
            componentBytes[16], componentBytes[17], componentBytes[18], componentBytes[19],
            componentBytes[20], componentBytes[21], componentBytes[22], componentBytes[23],
            componentBytes[24], componentBytes[25], componentBytes[26], componentBytes[27],
            componentBytes[28], componentBytes[29], componentBytes[30], componentBytes[31],
            componentBytes[32], componentBytes[33], componentBytes[34], componentBytes[35],
            componentBytes[36], componentBytes[37], componentBytes[38], componentBytes[39],
            stackText);
        DumpCouchSequenceOwner(matchNumber, component);
        bool expected = false;
        if (g_sequenceLevelEntityCouchLogged.compare_exchange_strong(
                expected, true, std::memory_order_relaxed)) {
            Log("Trailblazer couch level entity resolved phase=%s component=%p "
                "archive=%p caller_rva=0x%llX uid_layout=%s",
                phase, component, archive,
                static_cast<unsigned long long>(ToRva(caller)), layout);
        }
    }
    StateTrace("SequenceLevelEntity.uid", component, archive,
        "phase=%s;caller_rva=0x%llX;uid_raw="
        "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X;"
        "couch=%u;layout=%s",
        phase, static_cast<unsigned long long>(ToRva(caller)),
        uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6], uid[7],
        uid[8], uid[9], uid[10], uid[11], uid[12], uid[13], uid[14], uid[15],
        layout ? 1u : 0u, layout ? layout : "none");
}

void DumpCouchCollectionLoad(void* sequenceBlock, void* archive, void* component,
    bool result, void* caller, uint64_t generationDelta) {
    const uint64_t loadNumber =
        g_sequenceBlockCouchLoads.fetch_add(1, std::memory_order_relaxed) + 1;
    uint8_t blockBytes[0x200] = {};
    const bool blockReadable = sequenceBlock &&
        ReadDirectValue(sequenceBlock, blockBytes, sizeof(blockBytes));
    void* vtable = nullptr;
    void* componentOwner = nullptr;
    void* componentStorage = nullptr;
    uint32_t componentCount = 0;
    uint32_t componentCapacity = 0;
    if (blockReadable) {
        memcpy(&vtable, blockBytes, sizeof(vtable));
        memcpy(&componentStorage, blockBytes + 0x60, sizeof(componentStorage));
        memcpy(&componentCount, blockBytes + 0x68, sizeof(componentCount));
        memcpy(&componentCapacity, blockBytes + 0x6C, sizeof(componentCapacity));
    }
    if (component) {
        ReadDirectValue(static_cast<uint8_t*>(component) + 0x10,
            &componentOwner, sizeof(componentOwner));
    }

    char occurrenceText[384] = {};
    int occurrenceUsed = 0;
    size_t occurrenceCount = 0;
    if (blockReadable && component) {
        for (size_t offset = 0; offset + sizeof(void*) <= sizeof(blockBytes);
            offset += sizeof(void*)) {
            void* value = nullptr;
            memcpy(&value, blockBytes + offset, sizeof(value));
            if (value != component) continue;
            ++occurrenceCount;
            if (occurrenceUsed >= 0 &&
                static_cast<size_t>(occurrenceUsed) < sizeof(occurrenceText)) {
                occurrenceUsed += snprintf(occurrenceText + occurrenceUsed,
                    sizeof(occurrenceText) - static_cast<size_t>(occurrenceUsed),
                    "%s+0x%llX", occurrenceUsed ? "," : "",
                    static_cast<unsigned long long>(offset));
            }
        }
    }

    char vectorText[1024] = {};
    int vectorUsed = 0;
    const uint32_t boundedCount = componentCount < 32 ? componentCount : 32;
    bool componentInVector = false;
    for (uint32_t index = 0; componentStorage && index < boundedCount; ++index) {
        void* value = nullptr;
        if (!ReadDirectValue(static_cast<uint8_t*>(componentStorage) +
                static_cast<size_t>(index) * sizeof(void*), &value, sizeof(value))) break;
        if (value == component) componentInVector = true;
        if (vectorUsed >= 0 && static_cast<size_t>(vectorUsed) < sizeof(vectorText)) {
            vectorUsed += snprintf(vectorText + vectorUsed,
                sizeof(vectorText) - static_cast<size_t>(vectorUsed),
                "%s%u:%p", vectorUsed ? "," : "", index, value);
        }
    }

    Log("couch collection load=%llu result=%s generation_delta=%llu caller_rva=0x%llX "
        "block=%p block_readable=%s vtable=%p vtable_rva=0x%llX archive=%p "
        "component=%p component_owner=%p storage=%p count=%u capacity=%u "
        "component_in_vector=%s inline_occurrences=%llu offsets=%s vector=%s",
        static_cast<unsigned long long>(loadNumber), result ? "TRUE" : "FALSE",
        static_cast<unsigned long long>(generationDelta),
        static_cast<unsigned long long>(ToRva(caller)), sequenceBlock,
        blockReadable ? "YES" : "NO", vtable,
        static_cast<unsigned long long>(ToRva(vtable)), archive, component,
        componentOwner, componentStorage, componentCount, componentCapacity,
        componentInVector ? "YES" : "NO",
        static_cast<unsigned long long>(occurrenceCount), occurrenceText, vectorText);

    if (!blockReadable) return;
    const uintptr_t* words = reinterpret_cast<const uintptr_t*>(blockBytes);
    constexpr size_t wordsPerLine = 16;
    constexpr size_t wordCount = sizeof(blockBytes) / sizeof(uintptr_t);
    for (size_t baseWord = 0; baseWord < wordCount; baseWord += wordsPerLine) {
        char line[2048] = {};
        int used = snprintf(line, sizeof(line),
            "couch collection load=%llu block words",
            static_cast<unsigned long long>(loadNumber));
        const size_t endWord = (baseWord + wordsPerLine < wordCount)
            ? baseWord + wordsPerLine : wordCount;
        for (size_t index = baseWord; index < endWord && used >= 0 &&
            static_cast<size_t>(used) < sizeof(line); ++index) {
            used += snprintf(line + used, sizeof(line) - static_cast<size_t>(used),
                " +%03llX=%p", static_cast<unsigned long long>(index * sizeof(uintptr_t)),
                reinterpret_cast<void*>(words[index]));
        }
        Log("%s", line);
    }
}

bool DumpCouchTargetTrackPointer(const char* scope, uint64_t loadNumber,
    void* address, size_t requestedBytes) {
    uint8_t bytes[0x100] = {};
    const size_t byteCount = requestedBytes < sizeof(bytes)
        ? requestedBytes : sizeof(bytes);
    const bool readable = address && ReadDirectValue(address, bytes, byteCount);
    void* first = nullptr;
    if (readable && byteCount >= sizeof(first)) memcpy(&first, bytes, sizeof(first));
    Log("couch Target Track load=%llu %s address=%p readable=%s bytes=0x%llX "
        "first=%p first_rva=0x%llX",
        static_cast<unsigned long long>(loadNumber), scope, address,
        readable ? "YES" : "NO", static_cast<unsigned long long>(byteCount),
        first, static_cast<unsigned long long>(ToRva(first)));
    if (!readable) return false;

    const uintptr_t* words = reinterpret_cast<const uintptr_t*>(bytes);
    constexpr size_t wordsPerLine = 8;
    const size_t wordCount = byteCount / sizeof(uintptr_t);
    for (size_t baseWord = 0; baseWord < wordCount; baseWord += wordsPerLine) {
        char line[2048] = {};
        int used = snprintf(line, sizeof(line),
            "couch Target Track load=%llu %s words",
            static_cast<unsigned long long>(loadNumber), scope);
        const size_t endWord = (baseWord + wordsPerLine < wordCount)
            ? baseWord + wordsPerLine : wordCount;
        for (size_t index = baseWord; index < endWord && used >= 0 &&
            static_cast<size_t>(used) < sizeof(line); ++index) {
            used += snprintf(line + used, sizeof(line) - static_cast<size_t>(used),
                " +%03llX=%p",
                static_cast<unsigned long long>(index * sizeof(uintptr_t)),
                reinterpret_cast<void*>(words[index]));
        }
        Log("%s", line);
    }
    return true;
}

void DumpCouchTargetTrackLoad(void* sequenceBlock, void* archive, void* context,
    void* component, bool result, void* caller, uint64_t generationDelta,
    const uint8_t blockBefore[0x200], bool blockBeforeReadable,
    const uint8_t contextBefore[0x100], bool contextBeforeReadable) {
    const uint64_t loadNumber =
        g_targetTrackCouchLoads.fetch_add(1, std::memory_order_relaxed) + 1;
    uint8_t blockAfter[0x200] = {};
    uint8_t contextAfter[0x100] = {};
    const bool blockAfterReadable = sequenceBlock &&
        ReadDirectValue(sequenceBlock, blockAfter, sizeof(blockAfter));
    const bool contextAfterReadable = context &&
        ReadDirectValue(context, contextAfter, sizeof(contextAfter));

    void* vtable = nullptr;
    void* componentOwner = nullptr;
    void* componentStorage = nullptr;
    uint32_t componentCount = 0;
    uint32_t componentCapacity = 0;
    if (blockAfterReadable) {
        memcpy(&vtable, blockAfter, sizeof(vtable));
        memcpy(&componentStorage, blockAfter + 0x60, sizeof(componentStorage));
        memcpy(&componentCount, blockAfter + 0x68, sizeof(componentCount));
        memcpy(&componentCapacity, blockAfter + 0x6C, sizeof(componentCapacity));
    }
    if (component) {
        ReadDirectValue(static_cast<uint8_t*>(component) + 0x10,
            &componentOwner, sizeof(componentOwner));
    }

    char vectorText[1024] = {};
    int vectorUsed = 0;
    bool componentInVector = false;
    const uint32_t boundedCount = componentCount < 32 ? componentCount : 32;
    for (uint32_t index = 0; componentStorage && index < boundedCount; ++index) {
        void* value = nullptr;
        if (!ReadDirectValue(static_cast<uint8_t*>(componentStorage) +
                static_cast<size_t>(index) * sizeof(void*), &value, sizeof(value))) break;
        if (value == component) componentInVector = true;
        if (vectorUsed >= 0 && static_cast<size_t>(vectorUsed) < sizeof(vectorText)) {
            vectorUsed += snprintf(vectorText + vectorUsed,
                sizeof(vectorText) - static_cast<size_t>(vectorUsed),
                "%s%u:%p", vectorUsed ? "," : "", index, value);
        }
    }

    size_t changedBlockWords = 0;
    char changedOffsets[1024] = {};
    int changedUsed = 0;
    if (blockBeforeReadable && blockAfterReadable) {
        for (size_t offset = 0; offset < sizeof(blockAfter); offset += sizeof(uintptr_t)) {
            if (memcmp(blockBefore + offset, blockAfter + offset, sizeof(uintptr_t)) == 0)
                continue;
            ++changedBlockWords;
            if (changedUsed >= 0 &&
                static_cast<size_t>(changedUsed) < sizeof(changedOffsets)) {
                changedUsed += snprintf(changedOffsets + changedUsed,
                    sizeof(changedOffsets) - static_cast<size_t>(changedUsed),
                    "%s+0x%03llX", changedUsed ? "," : "",
                    static_cast<unsigned long long>(offset));
            }
        }
    }

    void* stack[12] = {};
    const USHORT stackCount = CaptureStackBackTrace(2, 12, stack, nullptr);
    char stackText[512] = {};
    int stackUsed = 0;
    for (USHORT index = 0; index < stackCount && stackUsed >= 0 &&
        static_cast<size_t>(stackUsed) < sizeof(stackText); ++index) {
        stackUsed += snprintf(stackText + stackUsed,
            sizeof(stackText) - static_cast<size_t>(stackUsed), "%s0x%llX",
            stackUsed ? "," : "",
            static_cast<unsigned long long>(ToRva(stack[index])));
    }

    Log("couch Target Track load=%llu result=%s generation_delta=%llu "
        "caller_rva=0x%llX block=%p before_readable=%s after_readable=%s "
        "vtable=%p vtable_rva=0x%llX archive=%p context=%p "
        "context_before_readable=%s context_after_readable=%s component=%p "
        "component_owner=%p storage=%p count=%u capacity=%u "
        "component_in_vector=%s changed_block_words=%llu changed_offsets=%s "
        "vector=%s stack_rvas=%s",
        static_cast<unsigned long long>(loadNumber), result ? "TRUE" : "FALSE",
        static_cast<unsigned long long>(generationDelta),
        static_cast<unsigned long long>(ToRva(caller)), sequenceBlock,
        blockBeforeReadable ? "YES" : "NO", blockAfterReadable ? "YES" : "NO",
        vtable, static_cast<unsigned long long>(ToRva(vtable)), archive, context,
        contextBeforeReadable ? "YES" : "NO", contextAfterReadable ? "YES" : "NO",
        component, componentOwner, componentStorage, componentCount, componentCapacity,
        componentInVector ? "YES" : "NO",
        static_cast<unsigned long long>(changedBlockWords), changedOffsets,
        vectorText, stackText);

    if (blockBeforeReadable || blockAfterReadable) {
        const uintptr_t* beforeWords =
            reinterpret_cast<const uintptr_t*>(blockBefore);
        const uintptr_t* afterWords =
            reinterpret_cast<const uintptr_t*>(blockAfter);
        constexpr size_t wordsPerLine = 8;
        constexpr size_t wordCount = sizeof(blockAfter) / sizeof(uintptr_t);
        for (size_t baseWord = 0; baseWord < wordCount; baseWord += wordsPerLine) {
            char line[2048] = {};
            int used = snprintf(line, sizeof(line),
                "couch Target Track load=%llu block before/after",
                static_cast<unsigned long long>(loadNumber));
            const size_t endWord = (baseWord + wordsPerLine < wordCount)
                ? baseWord + wordsPerLine : wordCount;
            for (size_t index = baseWord; index < endWord && used >= 0 &&
                static_cast<size_t>(used) < sizeof(line); ++index) {
                used += snprintf(line + used, sizeof(line) - static_cast<size_t>(used),
                    " +%03llX=%p->%p",
                    static_cast<unsigned long long>(index * sizeof(uintptr_t)),
                    reinterpret_cast<void*>(beforeWords[index]),
                    reinterpret_cast<void*>(afterWords[index]));
            }
            Log("%s", line);
        }
    }

    if (contextBeforeReadable || contextAfterReadable) {
        const uintptr_t* beforeWords =
            reinterpret_cast<const uintptr_t*>(contextBefore);
        const uintptr_t* afterWords =
            reinterpret_cast<const uintptr_t*>(contextAfter);
        constexpr size_t wordsPerLine = 8;
        constexpr size_t wordCount = sizeof(contextAfter) / sizeof(uintptr_t);
        for (size_t baseWord = 0; baseWord < wordCount; baseWord += wordsPerLine) {
            char line[2048] = {};
            int used = snprintf(line, sizeof(line),
                "couch Target Track load=%llu context before/after",
                static_cast<unsigned long long>(loadNumber));
            const size_t endWord = (baseWord + wordsPerLine < wordCount)
                ? baseWord + wordsPerLine : wordCount;
            for (size_t index = baseWord; index < endWord && used >= 0 &&
                static_cast<size_t>(used) < sizeof(line); ++index) {
                used += snprintf(line + used, sizeof(line) - static_cast<size_t>(used),
                    " +%03llX=%p->%p",
                    static_cast<unsigned long long>(index * sizeof(uintptr_t)),
                    reinterpret_cast<void*>(beforeWords[index]),
                    reinterpret_cast<void*>(afterWords[index]));
            }
            Log("%s", line);
        }
    }

    DumpCouchTargetTrackPointer("archive record", loadNumber, archive, 0x100);

    if (!blockAfterReadable) return;
    void* shared1B0 = nullptr;
    void* shared1C0 = nullptr;
    void* trackVectorStorage = nullptr;
    uint32_t trackVectorCount = 0;
    uint32_t trackVectorCapacity = 0;
    memcpy(&trackVectorStorage, blockAfter + 0x170, sizeof(trackVectorStorage));
    memcpy(&trackVectorCount, blockAfter + 0x178, sizeof(trackVectorCount));
    memcpy(&trackVectorCapacity, blockAfter + 0x17C, sizeof(trackVectorCapacity));
    memcpy(&shared1B0, blockAfter + 0x1B0, sizeof(shared1B0));
    memcpy(&shared1C0, blockAfter + 0x1C0, sizeof(shared1C0));

    Log("couch Target Track load=%llu finalized links shared_1B0=%p "
        "shared_1C0=%p shared_equal=%s track_storage=%p track_count=%u "
        "track_capacity=%u",
        static_cast<unsigned long long>(loadNumber), shared1B0, shared1C0,
        shared1B0 && shared1B0 == shared1C0 ? "YES" : "NO",
        trackVectorStorage, trackVectorCount, trackVectorCapacity);
    DumpCouchTargetTrackPointer("shared +1B0", loadNumber, shared1B0, 0x100);
    if (shared1C0 != shared1B0)
        DumpCouchTargetTrackPointer("shared +1C0", loadNumber, shared1C0, 0x100);

    DumpCouchTargetTrackPointer("track vector storage", loadNumber,
        trackVectorStorage, 0x100);
    const uint32_t boundedTrackCount = trackVectorCount < 16
        ? trackVectorCount : 16;
    for (uint32_t index = 0; trackVectorStorage && index < boundedTrackCount; ++index) {
        uint8_t record[0x10] = {};
        const bool recordRead = ReadDirectValue(
            static_cast<uint8_t*>(trackVectorStorage) +
                static_cast<size_t>(index) * sizeof(record),
            record, sizeof(record));
        if (!recordRead) {
            Log("couch Target Track load=%llu track record %u unreadable=YES",
                static_cast<unsigned long long>(loadNumber), index);
            break;
        }
        uint64_t packed = 0;
        void* entry = nullptr;
        memcpy(&packed, record, sizeof(packed));
        memcpy(&entry, record + sizeof(packed), sizeof(entry));
        const uint32_t packedIndex = static_cast<uint32_t>(packed);
        const uint32_t packedType = static_cast<uint32_t>(packed >> 32);
        Log("couch Target Track load=%llu track record=%u packed=0x%016llX "
            "index=%u type=0x%08X object=%p",
            static_cast<unsigned long long>(loadNumber), index,
            static_cast<unsigned long long>(packed), packedIndex, packedType, entry);
        char scope[64] = {};
        snprintf(scope, sizeof(scope), "track record %u object", index);
        DumpCouchTargetTrackPointer(scope, loadNumber, entry, 0x80);
    }
}

uint32_t FloatBits(float value) {
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

float BitsFloat(uint32_t bits) {
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

int32_t HalfMeterBucket(float value) {
    if (value > 1000000.0f) return 2000000;
    if (value < -1000000.0f) return -2000000;
    return static_cast<int32_t>(value * 2.0f);
}

bool SnapshotEqual(const AimSnapshot& left, const AimSnapshot& right) {
    return left.readableMask == right.readableMask &&
        left.output20 == right.output20 && left.output30 == right.output30 &&
        left.output40 == right.output40 && left.distanceHalfMeters == right.distanceHalfMeters &&
        left.output28 == right.output28 && left.output48 == right.output48;
}

bool SnapshotEqual(const DamageSnapshot& left, const DamageSnapshot& right) {
    return left.readableMask == right.readableMask &&
        left.output30 == right.output30 && left.output40 == right.output40 &&
        left.output48 == right.output48 && left.output50 == right.output50 &&
        left.output58 == right.output58 && left.output70 == right.output70 &&
        left.output78 == right.output78 && left.output88 == right.output88 &&
        left.output60 == right.output60 && left.output68 == right.output68 &&
        left.output80 == right.output80 && left.output28 == right.output28 &&
        left.output38 == right.output38 && left.output90 == right.output90;
}

bool SnapshotEqual(const HostileSnapshot& left, const HostileSnapshot& right) {
    return left.readableMask == right.readableMask && left.output20 == right.output20;
}

bool SnapshotEqual(const GraphicsSnapshot& left, const GraphicsSnapshot& right) {
    return left.readableMask == right.readableMask && left.output6C == right.output6C &&
        left.nodeReady28 == right.nodeReady28 &&
        left.contextStatus824 == right.contextStatus824 && left.output18 == right.output18;
}

bool SnapshotEqual(const InteractionSnapshot& left, const InteractionSnapshot& right) {
    return left.readableMask == right.readableMask && left.input20 == right.input20 &&
        left.runtime840 == right.runtime840 &&
        left.executionState858 == right.executionState858;
}

bool SnapshotEqual(const OnDemandSnapshot& left, const OnDemandSnapshot& right) {
    return left.readableMask == right.readableMask && left.child58 == right.child58 &&
        left.owner60 == right.owner60 && left.state68 == right.state68 &&
        left.pushType6C == right.pushType6C;
}

template <typename Snapshot>
bool UpdateStateCache(StateCache<Snapshot>& cache, void* self, uint32_t readableMask,
    const Snapshot& snapshot, Snapshot* previous = nullptr,
    bool* hadPrevious = nullptr) {
    bool changed = false;
    if (previous) *previous = {};
    if (hadPrevious) *hadPrevious = false;
    AcquireSRWLockExclusive(&cache.lock);
    size_t index = 0;
    for (; index < cache.count; ++index) {
        if (cache.entries[index].self == self && cache.entries[index].readableMask == readableMask) break;
    }
    if (index == cache.count) {
        if (cache.count < kMaxTrackedInstances) {
            cache.entries[cache.count].self = self;
            cache.entries[cache.count].readableMask = readableMask;
            cache.entries[cache.count].snapshot = snapshot;
            ++cache.count;
            changed = true;
        }
    } else {
        if (previous) *previous = cache.entries[index].snapshot;
        if (hadPrevious) *hadPrevious = true;
        if (!SnapshotEqual(cache.entries[index].snapshot, snapshot)) {
            cache.entries[index].snapshot = snapshot;
            changed = true;
        }
    }
    ReleaseSRWLockExclusive(&cache.lock);
    return changed;
}

template <typename Snapshot>
void ResetStateCache(StateCache<Snapshot>& cache) {
    AcquireSRWLockExclusive(&cache.lock);
    cache.count = 0;
    ReleaseSRWLockExclusive(&cache.lock);
}

void ResetUISetExecutionWindow() {
    AcquireSRWLockExclusive(&g_uiSetExecutionStates.lock);
    g_uiSetExecutionStates.count = 0;
    ReleaseSRWLockExclusive(&g_uiSetExecutionStates.lock);
    g_contractsSetExecutionWindow.store(false, std::memory_order_relaxed);
    g_contractsSetExecutionArmUs.store(0, std::memory_order_relaxed);
    g_contractsUIExecutionStateManager.store(nullptr, std::memory_order_relaxed);
    g_contractsUIExecutionStatePresence.store(0xFFFFFFFFu, std::memory_order_relaxed);
    g_contractsUIExecutionStatesSeen.store(false, std::memory_order_relaxed);
    g_contractsUIExecutionStatesRetired.store(false, std::memory_order_relaxed);
}

uint32_t CombinedContractsStatePresence(uint32_t presence) {
    uint32_t combined = 0;
    if (presence & ((1u << 0) | (1u << 2))) combined |= 1u << 0;
    if (presence & ((1u << 1) | (1u << 3))) combined |= 1u << 1;
    return combined;
}

bool ReadContractsStateManagerBits(void* manager, uint64_t* currentWord,
    uint64_t* pendingWord) {
    if (!manager || !currentWord || !pendingWord) return false;
    return ReadDirectValue(static_cast<uint8_t*>(manager) + 0xA0,
               currentWord, sizeof(*currentWord)) &&
        ReadDirectValue(static_cast<uint8_t*>(manager) + 0xB0,
            pendingWord, sizeof(*pendingWord));
}

uint32_t ContractsStatePresence(uint64_t currentWord, uint64_t pendingWord) {
    uint32_t presence = 0;
    if (currentWord & (1ull << kContractsTerminalStateId)) presence |= 1u << 0;
    if (currentWord & (1ull << kHideLocalPlayerStateId)) presence |= 1u << 1;
    if (pendingWord & (1ull << kContractsTerminalStateId)) presence |= 1u << 2;
    if (pendingWord & (1ull << kHideLocalPlayerStateId)) presence |= 1u << 3;
    return presence;
}

void SampleContractsStateManager(const char* source, void* manager,
    uint32_t callStateId, void* caller, bool force = false) {
    if (!manager || !g_capture.load(std::memory_order_relaxed) ||
        !g_contractsSetExecutionWindow.load(std::memory_order_relaxed) ||
        manager != g_contractsUIExecutionStateManager.load(std::memory_order_relaxed)) return;

    uint64_t currentWord = 0;
    uint64_t pendingWord = 0;
    if (!ReadContractsStateManagerBits(manager, &currentWord, &pendingWord)) return;
    const uint32_t presence = ContractsStatePresence(currentWord, pendingWord);
    const uint32_t previous = g_contractsUIExecutionStatePresence.exchange(
        presence, std::memory_order_relaxed);
    const uint32_t combined = CombinedContractsStatePresence(presence);
    const uint32_t previousCombined = previous == 0xFFFFFFFFu ? 0xFFFFFFFFu :
        CombinedContractsStatePresence(previous);
    if (combined == 0x3u)
        g_contractsUIExecutionStatesSeen.store(true, std::memory_order_relaxed);

    if (force || presence != previous) {
        StateTrace("UIExecutionStateManager.contracts.bits", manager, caller,
            "source=%s;call_state_id=%u;current_word=0x%llX;pending_word=0x%llX;"
            "presence=0x%X;previous=0x%X;combined=0x%X;caller_rva=0x%llX",
            source ? source : "unspecified", callStateId,
            static_cast<unsigned long long>(currentWord),
            static_cast<unsigned long long>(pendingWord), presence, previous, combined,
            static_cast<unsigned long long>(ToRva(caller)));
    }

    if (g_contractsUIExecutionStatesSeen.load(std::memory_order_relaxed) &&
        combined == 0 && previousCombined != 0 && previousCombined != 0xFFFFFFFFu &&
        !g_contractsUIExecutionStatesRetired.exchange(true, std::memory_order_relaxed)) {
        const uint64_t retiredUs = QpcMicroseconds();
        StateTrace("UIExecutionStateManager.contracts.retired", manager, caller,
            "source=%s;call_state_id=%u;retired_us=%llu;previous=0x%X;"
            "current_word=0x%llX;pending_word=0x%llX;caller_rva=0x%llX",
            source ? source : "unspecified", callStateId,
            static_cast<unsigned long long>(retiredUs), previous,
            static_cast<unsigned long long>(currentWord),
            static_cast<unsigned long long>(pendingWord),
            static_cast<unsigned long long>(ToRva(caller)));
    }
}

void SelectContractsStateManager(void* manager, uint32_t stateId, void* caller) {
    if (!manager) return;
    void* previous = g_contractsUIExecutionStateManager.exchange(
        manager, std::memory_order_relaxed);
    if (previous == manager) return;
    g_contractsUIExecutionStatePresence.store(0xFFFFFFFFu, std::memory_order_relaxed);
    g_contractsUIExecutionStatesSeen.store(false, std::memory_order_relaxed);
    g_contractsUIExecutionStatesRetired.store(false, std::memory_order_relaxed);
    StateTrace("UIExecutionStateManager.contracts.select", manager, caller,
        "state_id=%u;previous_manager=%p;caller_rva=0x%llX", stateId, previous,
        static_cast<unsigned long long>(ToRva(caller)));
}

void DumpContractsStateManager(const char* reason) {
    void* manager = g_contractsUIExecutionStateManager.load(std::memory_order_relaxed);
    if (!manager || !g_capture.load(std::memory_order_relaxed) ||
        !g_contractsSetExecutionWindow.load(std::memory_order_relaxed)) return;
    SampleContractsStateManager(reason, manager, 0xFFFFFFFFu, nullptr, true);
}

void ArmUISetExecutionWindow(void* self, void* context) {
    if (!g_capture.load(std::memory_order_relaxed)) return;
    bool expected = false;
    if (!g_contractsSetExecutionWindow.compare_exchange_strong(
            expected, true, std::memory_order_relaxed)) return;
    const uint64_t armUs = QpcMicroseconds();
    g_contractsSetExecutionArmUs.store(armUs, std::memory_order_relaxed);
    StateTrace("UISetExecutionState.contracts_window.arm", self, context,
        "arm_us=%llu", static_cast<unsigned long long>(armUs));
}

void CaptureUISetExecutionStateCall(void* self, void* context, void* caller) {
    if (!self || !g_capture.load(std::memory_order_relaxed) ||
        !g_contractsSetExecutionWindow.load(std::memory_order_relaxed)) return;

    UISetExecutionEntry observed = {};
    observed.self = self;
    observed.context = context;
    observed.firstUs = QpcMicroseconds();
    observed.lastUs = observed.firstUs;
    observed.callCount = 1;
    observed.callerRva = ToRva(caller);
    if (ReadDirectValue(static_cast<uint8_t*>(self) + 0x28,
            &observed.stateObject28, sizeof(observed.stateObject28)))
        observed.readableMask |= 1u << 0;
    if (ReadDirectValue(static_cast<uint8_t*>(self) + 0x30,
            &observed.stateId30, sizeof(observed.stateId30)))
        observed.readableMask |= 1u << 1;
    if (ReadDirectValue(static_cast<uint8_t*>(self) + 0x38,
            &observed.trailing38, sizeof(observed.trailing38)))
        observed.readableMask |= 1u << 2;

    bool first = false;
    AcquireSRWLockExclusive(&g_uiSetExecutionStates.lock);
    size_t index = 0;
    for (; index < g_uiSetExecutionStates.count; ++index) {
        if (g_uiSetExecutionStates.entries[index].self == self) break;
    }
    if (index == g_uiSetExecutionStates.count &&
        g_uiSetExecutionStates.count < kMaxUISetExecutionEntries) {
        g_uiSetExecutionStates.entries[g_uiSetExecutionStates.count++] = observed;
        first = true;
    } else if (index < g_uiSetExecutionStates.count) {
        auto& entry = g_uiSetExecutionStates.entries[index];
        entry.context = context;
        entry.lastUs = observed.lastUs;
        ++entry.callCount;
        entry.readableMask = observed.readableMask;
        entry.stateObject28 = observed.stateObject28;
        entry.stateId30 = observed.stateId30;
        entry.trailing38 = observed.trailing38;
    }
    ReleaseSRWLockExclusive(&g_uiSetExecutionStates.lock);

    if (!first) return;
    Trace("UISetExecutionState.contracts_window.first", self, context, caller);
    StateTrace("UISetExecutionState.contracts_window.first", self, context,
        "mask=0x%X;state_object28=0x%llX;state_id30=%u;trailing38=0x%llX;caller_rva=0x%llX",
        observed.readableMask,
        static_cast<unsigned long long>(observed.stateObject28), observed.stateId30,
        static_cast<unsigned long long>(observed.trailing38),
        static_cast<unsigned long long>(observed.callerRva));
}

void DumpUISetExecutionWindow(const char* reason) {
    if (!g_capture.load(std::memory_order_relaxed) ||
        !g_contractsSetExecutionWindow.load(std::memory_order_relaxed)) return;
    const uint64_t armUs = g_contractsSetExecutionArmUs.load(std::memory_order_relaxed);
    AcquireSRWLockShared(&g_uiSetExecutionStates.lock);
    for (size_t index = 0; index < g_uiSetExecutionStates.count; ++index) {
        const auto& entry = g_uiSetExecutionStates.entries[index];
        StateTrace("UISetExecutionState.contracts_window.summary", entry.self,
            entry.context,
            "reason=%s;mask=0x%X;state_object28=0x%llX;state_id30=%u;"
            "trailing38=0x%llX;calls=%llu;arm_us=%llu;first_us=%llu;last_us=%llu;"
            "first_after_arm_us=%llu;caller_rva=0x%llX",
            reason ? reason : "unspecified", entry.readableMask,
            static_cast<unsigned long long>(entry.stateObject28), entry.stateId30,
            static_cast<unsigned long long>(entry.trailing38),
            static_cast<unsigned long long>(entry.callCount),
            static_cast<unsigned long long>(armUs),
            static_cast<unsigned long long>(entry.firstUs),
            static_cast<unsigned long long>(entry.lastUs),
            static_cast<unsigned long long>(entry.firstUs >= armUs ? entry.firstUs - armUs : 0),
            static_cast<unsigned long long>(entry.callerRva));
    }
    const size_t count = g_uiSetExecutionStates.count;
    ReleaseSRWLockShared(&g_uiSetExecutionStates.lock);
    Log("UI:SetExecutionState contracts-window summary reason=%s entries=%llu",
        reason ? reason : "unspecified", static_cast<unsigned long long>(count));
}

void ResetStateCaches() {
    ResetStateCache(g_aimStates);
    ResetStateCache(g_damageStates);
    ResetStateCache(g_hostileStates);
    ResetStateCache(g_lineStates);
    ResetStateCache(g_multiStates);
    ResetStateCache(g_interactionListenerStates);
    ResetStateCache(g_interactionModifierStates);
    ResetStateCache(g_uiExecutionStateStates);
    ResetStateCache(g_menuGraphStackOnDemandStates);
    ResetUISetExecutionWindow();
    g_nixBowlGraphOpenCalls.store(0, std::memory_order_relaxed);
    g_nixBowlGraphCloseCalls.store(0, std::memory_order_relaxed);
    g_nixBowlInnerOpenCalls.store(0, std::memory_order_relaxed);
    g_nixBowlInnerCloseCalls.store(0, std::memory_order_relaxed);
    g_nixBowlInnerCallerMismatches.store(0, std::memory_order_relaxed);
}

void StateTrace(const char* node, void* self, void* context, const char* format, ...) {
    if (!g_capture.load(std::memory_order_relaxed) || !g_states) return;
    char details[1024] = {};
    va_list args;
    va_start(args, format);
    vsnprintf_s(details, sizeof(details), _TRUNCATE, format, args);
    va_end(args);
    const uint64_t sequence = g_stateSequence.fetch_add(1, std::memory_order_relaxed) + 1;
    EnterCriticalSection(&g_logLock);
    fprintf(g_states, "%llu\t%llu\t%lu\t%s\t%p\t%p\t%s\n",
        static_cast<unsigned long long>(sequence),
        static_cast<unsigned long long>(QpcMicroseconds()),
        GetCurrentThreadId(), node, self, context, details);
    LeaveCriticalSection(&g_logLock);
}

void CaptureAimState(void* self, void* context) {
    if (!g_capture.load(std::memory_order_relaxed)) return;
    AimSnapshot snapshot = {};
    float position[3] = {};
    float distance = 0.0f;
    if (ReadSlotValue(context, 0x20, 0x2223D80, &snapshot.output20, sizeof(snapshot.output20))) snapshot.readableMask |= 1u << 0;
    if (ReadSlotValue(context, 0x28, kRvaGetBoolStorage, &snapshot.output28, sizeof(snapshot.output28))) snapshot.readableMask |= 1u << 1;
    if (ReadSlotValue(context, 0x30, 0x337000, &snapshot.output30, sizeof(snapshot.output30))) snapshot.readableMask |= 1u << 2;
    if (ReadSlotValue(context, 0x38, kRvaGetVec3Storage, position, sizeof(position))) snapshot.readableMask |= 1u << 3;
    if (ReadSlotValue(context, 0x40, 0x286CE10, &snapshot.output40, sizeof(snapshot.output40))) snapshot.readableMask |= 1u << 4;
    if (ReadSlotValue(context, 0x48, kRvaGetBoolStorage, &snapshot.output48, sizeof(snapshot.output48))) snapshot.readableMask |= 1u << 5;
    if (ReadSlotValue(context, 0x50, kRvaGetFloatStorage, &distance, sizeof(distance))) {
        snapshot.readableMask |= 1u << 6;
        snapshot.distanceHalfMeters = HalfMeterBucket(distance);
    }
    if (!UpdateStateCache(g_aimStates, self, snapshot.readableMask, snapshot)) return;
    StateTrace("PlayerAimTarget.state", self, context,
        "mask=0x%02X;o20_ref=0x%llX;o28_bool=%u;o30_ref=0x%llX;"
        "o38_vec3=%.4f,%.4f,%.4f;o40_ref=0x%llX;o48_bool=%u;o50_float=%.4f",
        snapshot.readableMask,
        static_cast<unsigned long long>(snapshot.output20), snapshot.output28,
        static_cast<unsigned long long>(snapshot.output30),
        position[0], position[1], position[2],
        static_cast<unsigned long long>(snapshot.output40), snapshot.output48, distance);
}

void CaptureDamageState(void* self, void* context) {
    if (!g_capture.load(std::memory_order_relaxed)) return;
    RegisterDamageSlots(self, context);
    DamageSnapshot snapshot = {};
    float value = 0.0f;
    if (ReadSlotValue(context, 0x28, kRvaGetBoolStorage, &snapshot.output28, sizeof(snapshot.output28))) snapshot.readableMask |= 1u << 0;
    if (ReadSlotValue(context, 0x30, kRvaGetFloatStorage, &value, sizeof(value))) { snapshot.output30 = FloatBits(value); snapshot.readableMask |= 1u << 1; }
    if (ReadSlotValue(context, 0x38, kRvaGetBoolStorage, &snapshot.output38, sizeof(snapshot.output38))) snapshot.readableMask |= 1u << 2;
    if (ReadSlotValue(context, 0x40, kRvaGetFloatStorage, &value, sizeof(value))) { snapshot.output40 = FloatBits(value); snapshot.readableMask |= 1u << 3; }
    if (ReadSlotValue(context, 0x48, kRvaGetFloatStorage, &value, sizeof(value))) { snapshot.output48 = FloatBits(value); snapshot.readableMask |= 1u << 4; }
    if (ReadSlotValue(context, 0x50, kRvaGetDamageU32AStorage, &snapshot.output50, sizeof(snapshot.output50))) snapshot.readableMask |= 1u << 5;
    if (ReadSlotValue(context, 0x58, kRvaGetDamageU32BStorage, &snapshot.output58, sizeof(snapshot.output58))) snapshot.readableMask |= 1u << 6;
    if (ReadSlotValue(context, 0x60, kRvaGetDamageRefAStorage, &snapshot.output60, sizeof(snapshot.output60))) snapshot.readableMask |= 1u << 7;
    if (ReadSlotValue(context, 0x68, kRvaGetDamageRefBStorage, &snapshot.output68, sizeof(snapshot.output68))) snapshot.readableMask |= 1u << 8;
    if (ReadSlotValue(context, 0x70, kRvaGetFloatStorage, &value, sizeof(value))) { snapshot.output70 = FloatBits(value); snapshot.readableMask |= 1u << 9; }
    if (ReadSlotValue(context, 0x78, kRvaGetFloatStorage, &value, sizeof(value))) { snapshot.output78 = FloatBits(value); snapshot.readableMask |= 1u << 10; }
    if (ReadSlotValue(context, 0x80, kRvaGetDamageRefCStorage, &snapshot.output80, sizeof(snapshot.output80))) snapshot.readableMask |= 1u << 11;
    if (ReadSlotValue(context, 0x88, kRvaGetDamageU32CStorage, &snapshot.output88, sizeof(snapshot.output88))) snapshot.readableMask |= 1u << 12;
    if (ReadSlotValue(context, 0x90, kRvaGetBoolStorage, &snapshot.output90, sizeof(snapshot.output90))) snapshot.readableMask |= 1u << 13;
    if ((snapshot.readableMask & 1u) && snapshot.output28) ArmDamageConsumerTrace();
    if (UpdateStateCache(g_damageStates, self, snapshot.readableMask, snapshot)) {
        StateTrace("DamageDealt.state", self, context,
            "mask=0x%04X;o28_bool=%u;o30_float=%.5f;o38_bool=%u;o40_float=%.5f;"
            "o48_float=%.5f;o50_u32=%u;o58_u32=%u;o60_ref=0x%llX;o68_ref=0x%llX;"
            "o70_float=%.5f;o78_float=%.5f;o80_ref=0x%llX;o88_u32=%u;o90_bool=%u",
            snapshot.readableMask, snapshot.output28, BitsFloat(snapshot.output30), snapshot.output38,
            BitsFloat(snapshot.output40), BitsFloat(snapshot.output48), snapshot.output50, snapshot.output58,
            static_cast<unsigned long long>(snapshot.output60),
            static_cast<unsigned long long>(snapshot.output68),
            BitsFloat(snapshot.output70), BitsFloat(snapshot.output78),
            static_cast<unsigned long long>(snapshot.output80), snapshot.output88, snapshot.output90);
    }

    constexpr uint32_t kCrosshairDamageFeedbackMask = 0x3B4D;
    if ((snapshot.readableMask & kCrosshairDamageFeedbackMask) == kCrosshairDamageFeedbackMask) {
        const uint8_t disabled = 0;
        if (g_gateHasDealtDamage.load(std::memory_order_relaxed))
            WriteSlotValue(context, 0x28, kRvaGetBoolStorage, &disabled, sizeof(disabled));
        if (g_gateFatal.load(std::memory_order_relaxed))
            WriteSlotValue(context, 0x38, kRvaGetBoolStorage, &disabled, sizeof(disabled));
    }
}

void CaptureHostileState(void* self, void* context) {
    if (!g_capture.load(std::memory_order_relaxed)) return;
    HostileSnapshot snapshot = {};
    if (ReadSlotValue(context, 0x20, kRvaGetBoolStorage, &snapshot.output20, sizeof(snapshot.output20))) snapshot.readableMask |= 1u;
    if (!UpdateStateCache(g_hostileStates, self, snapshot.readableMask, snapshot)) return;
    StateTrace("IsHostile.state", self, context, "mask=0x%X;o20_bool=%u",
        snapshot.readableMask, snapshot.output20);
}

void CaptureGraphicsState(void* self, void* context, bool lines) {
    if (!g_capture.load(std::memory_order_relaxed)) return;
    GraphicsSnapshot snapshot = {};
    if (ReadDirectValue(reinterpret_cast<uint8_t*>(self) + 0x28, &snapshot.nodeReady28, sizeof(snapshot.nodeReady28))) snapshot.readableMask |= 1u << 0;
    if (ReadDirectValue(reinterpret_cast<uint8_t*>(context) + 0x824, &snapshot.contextStatus824, sizeof(snapshot.contextStatus824))) snapshot.readableMask |= 1u << 1;
    void* output = nullptr;
    if (ReadDirectValue(reinterpret_cast<uint8_t*>(context) + 0x840, &output, sizeof(output)) && output) {
        if (ReadDirectValue(reinterpret_cast<uint8_t*>(output) + 0x18, &snapshot.output18, sizeof(snapshot.output18))) snapshot.readableMask |= 1u << 2;
        if (ReadDirectValue(reinterpret_cast<uint8_t*>(output) + 0x6C, &snapshot.output6C, sizeof(snapshot.output6C))) snapshot.readableMask |= 1u << 3;
    }
    auto& cache = lines ? g_lineStates : g_multiStates;
    if (!UpdateStateCache(cache, self, snapshot.readableMask, snapshot)) return;
    StateTrace(lines ? "GraphicsLines.state" : "GraphicsMultiSector.state", self, context,
        "mask=0x%X;node28=%u;context824=%u;output18=%u;output6C=%u",
        snapshot.readableMask, snapshot.nodeReady28, snapshot.contextStatus824,
        snapshot.output18, snapshot.output6C);
}

bool ReadInteractionSnapshot(void* context, InteractionSnapshot* snapshot) {
    if (!snapshot) return false;
    *snapshot = {};
    if (!context) return false;
    if (ReadDirectValue(reinterpret_cast<uint8_t*>(context) + 0x20,
            &snapshot->input20, sizeof(snapshot->input20))) {
        snapshot->readableMask |= 1u << 0;
    }
    if (ReadDirectValue(reinterpret_cast<uint8_t*>(context) + 0x840,
            &snapshot->runtime840, sizeof(snapshot->runtime840))) {
        snapshot->readableMask |= 1u << 1;
    }
    if (ReadDirectValue(reinterpret_cast<uint8_t*>(context) + 0x858,
            &snapshot->executionState858, sizeof(snapshot->executionState858))) {
        snapshot->readableMask |= 1u << 2;
    }
    return snapshot->readableMask != 0;
}

struct InteractionDiagnosticIdentity {
    uintptr_t selfVtableRva = 0;
    void* wrapper = nullptr;
    void* wrapperFirst = nullptr;
    void* callback = nullptr;
    uintptr_t callbackVtableRva = 0;
    uint64_t owner = 0;
    uintptr_t ownerVtableRva = 0;
    uint32_t nodeId = 0;
    uint32_t readableMask = 0;
    bool wrapperFromPrimary = false;
};

InteractionDiagnosticIdentity ResolveInteractionDiagnosticIdentity(void* self,
    uint64_t primaryRuntime, uint64_t fallbackRuntime) {
    InteractionDiagnosticIdentity identity = {};
    void** selfTable = nullptr;
    if (ReadVtable(self, &selfTable)) identity.selfVtableRva = ToRva(selfTable);
    identity.wrapperFromPrimary = primaryRuntime != 0;
    identity.wrapper = reinterpret_cast<void*>(
        identity.wrapperFromPrimary ? primaryRuntime : fallbackRuntime);
    if (!identity.wrapper || !ReadDirectValue(identity.wrapper,
            &identity.wrapperFirst, sizeof(identity.wrapperFirst))) {
        return identity;
    }
    identity.readableMask |= 1u << 0;
    void** callbackTable = nullptr;
    if (!identity.wrapperFirst || !ReadVtable(identity.wrapperFirst, &callbackTable)) {
        return identity;
    }
    identity.callbackVtableRva = ToRva(callbackTable);
    identity.readableMask |= 1u << 1;
    if (identity.callbackVtableRva != kRvaVtableInteractionCallback) return identity;
    identity.callback = identity.wrapperFirst;
    if (ReadDirectValue(reinterpret_cast<uint8_t*>(identity.callback) + 0x10,
            &identity.owner, sizeof(identity.owner))) {
        identity.readableMask |= 1u << 2;
        void** ownerTable = nullptr;
        if (identity.owner && ReadVtable(reinterpret_cast<void*>(identity.owner),
                &ownerTable)) {
            identity.ownerVtableRva = ToRva(ownerTable);
            identity.readableMask |= 1u << 4;
        }
    }
    if (ReadDirectValue(reinterpret_cast<uint8_t*>(identity.callback) + 0x18,
            &identity.nodeId, sizeof(identity.nodeId))) {
        identity.readableMask |= 1u << 3;
    }
    return identity;
}

__declspec(noinline) void FormatInteractionStackRvas(char* output,
    size_t outputSize, ULONG framesToSkip) {
    if (!output || !outputSize) return;
    output[0] = '\0';
    void* stack[8] = {};
    const USHORT stackCount = CaptureStackBackTrace(framesToSkip, 8, stack, nullptr);
    int used = 0;
    for (USHORT index = 0; index < stackCount && used >= 0 &&
        static_cast<size_t>(used) < outputSize; ++index) {
        used += snprintf(output + used, outputSize - static_cast<size_t>(used),
            "%s0x%llX", used ? "," : "",
            static_cast<unsigned long long>(ToRva(stack[index])));
    }
}

__declspec(noinline) void CaptureInteractionObservedState(void* self, void* context,
    NodeKind kind,
    const InteractionSnapshot& previous, const InteractionSnapshot& current,
    bool hadPrevious, void* caller) {
    if (!g_cfg.interactionObservedStateDetails ||
        !g_capture.load(std::memory_order_relaxed)) {
        return;
    }
    const bool listener = kind == NodeKind::InteractionListener;
    const bool modifier = kind == NodeKind::InteractionModifier;
    if (!listener && !modifier) return;
    if (listener) {
        g_interactionListenerObservations.fetch_add(1, std::memory_order_relaxed);
    } else {
        g_interactionModifierObservations.fetch_add(1, std::memory_order_relaxed);
    }

    const auto identity = ResolveInteractionDiagnosticIdentity(
        self, current.runtime840, previous.runtime840);
    char stackText[256] = {};
    FormatInteractionStackRvas(stackText, sizeof(stackText), 4);
    const bool executionChanged = hadPrevious &&
        (previous.readableMask & (1u << 2)) && (current.readableMask & (1u << 2)) &&
        previous.executionState858 != current.executionState858;
    const char* observationName = listener ?
        "InteractionListener.observed_state" : "InteractionModifier.observed_state";
    Trace(observationName, self, context, caller);
    StateTrace(observationName, self, context,
        "had_previous=%u;previous_mask=0x%X;current_mask=0x%X;"
        "previous_input20=0x%llX;current_input20=0x%llX;"
        "previous_runtime840=0x%llX;current_runtime840=0x%llX;"
        "previous_execution_state858=%u;current_execution_state858=%u;"
        "execution_changed=%u;caller_rva=0x%llX;self_vtable_rva=0x%llX;"
        "identity_mask=0x%X;wrapper=%p;wrapper_first=%p;wrapper_source=%s;"
        "callback=%p;callback_vtable_rva=0x%llX;owner=0x%llX;"
        "owner_vtable_rva=0x%llX;node_id=%u;stack_rvas=%s",
        hadPrevious ? 1u : 0u, previous.readableMask, current.readableMask,
        static_cast<unsigned long long>(previous.input20),
        static_cast<unsigned long long>(current.input20),
        static_cast<unsigned long long>(previous.runtime840),
        static_cast<unsigned long long>(current.runtime840),
        previous.executionState858, current.executionState858,
        executionChanged ? 1u : 0u,
        static_cast<unsigned long long>(ToRva(caller)),
        static_cast<unsigned long long>(identity.selfVtableRva),
        identity.readableMask, identity.wrapper, identity.wrapperFirst,
        identity.wrapperFromPrimary ? "current" : "previous", identity.callback,
        static_cast<unsigned long long>(identity.callbackVtableRva),
        static_cast<unsigned long long>(identity.owner),
        static_cast<unsigned long long>(identity.ownerVtableRva), identity.nodeId,
        stackText);
}

__declspec(noinline) void CaptureInteractionState(void* self, void* context, NodeKind kind,
    const InteractionSnapshot* suppliedSnapshot = nullptr, void* caller = nullptr) {
    if (!g_capture.load(std::memory_order_relaxed)) return;
    InteractionSnapshot snapshot = {};
    if (suppliedSnapshot) snapshot = *suppliedSnapshot;
    else ReadInteractionSnapshot(context, &snapshot);
    auto& cache = kind == NodeKind::InteractionListener ? g_interactionListenerStates :
        kind == NodeKind::InteractionModifier ? g_interactionModifierStates :
        g_uiExecutionStateStates;
    InteractionSnapshot previous = {};
    bool hadPrevious = false;
    if (!UpdateStateCache(cache, self, snapshot.readableMask, snapshot,
            &previous, &hadPrevious)) return;
    const char* stateName = kind == NodeKind::InteractionListener ? "InteractionListener.state" :
        kind == NodeKind::InteractionModifier ? "InteractionModifier.state" :
        "UIExecutionState.state";
    StateTrace(stateName,
        self, context, "mask=0x%X;input20=0x%llX;runtime840=0x%llX;execution_state858=%u",
        snapshot.readableMask, static_cast<unsigned long long>(snapshot.input20),
        static_cast<unsigned long long>(snapshot.runtime840), snapshot.executionState858);
    CaptureInteractionObservedState(self, context, kind, previous, snapshot,
        hadPrevious, caller);
}

__declspec(noinline) void CaptureInteractionTransition(void* self, void* context,
    NodeKind kind,
    const InteractionSnapshot& before, const InteractionSnapshot& after, void* caller) {
    if (!g_cfg.interactionTransitionDetails ||
        !g_capture.load(std::memory_order_relaxed) || SnapshotEqual(before, after)) {
        return;
    }

    const bool listener = kind == NodeKind::InteractionListener;
    const bool modifier = kind == NodeKind::InteractionModifier;
    if (!listener && !modifier) return;

    if (listener) {
        g_interactionListenerTransitions.fetch_add(1, std::memory_order_relaxed);
    } else {
        g_interactionModifierTransitions.fetch_add(1, std::memory_order_relaxed);
    }

    const auto identity = ResolveInteractionDiagnosticIdentity(
        self, after.runtime840, before.runtime840);
    char stackText[256] = {};
    FormatInteractionStackRvas(stackText, sizeof(stackText), 3);

    const bool executionChanged =
        (before.readableMask & (1u << 2)) && (after.readableMask & (1u << 2)) &&
        before.executionState858 != after.executionState858;
    const char* transitionName = listener ? "InteractionListener.eval_transition" :
        "InteractionModifier.eval_transition";
    Trace(transitionName, self, context, caller);
    StateTrace(transitionName, self, context,
        "before_mask=0x%X;after_mask=0x%X;before_input20=0x%llX;after_input20=0x%llX;"
        "before_runtime840=0x%llX;after_runtime840=0x%llX;before_execution_state858=%u;"
        "after_execution_state858=%u;execution_changed=%u;caller_rva=0x%llX;"
        "self_vtable_rva=0x%llX;identity_mask=0x%X;wrapper=%p;wrapper_first=%p;"
        "wrapper_source=%s;callback=%p;callback_vtable_rva=0x%llX;owner=0x%llX;"
        "owner_vtable_rva=0x%llX;node_id=%u;stack_rvas=%s",
        before.readableMask, after.readableMask,
        static_cast<unsigned long long>(before.input20),
        static_cast<unsigned long long>(after.input20),
        static_cast<unsigned long long>(before.runtime840),
        static_cast<unsigned long long>(after.runtime840),
        before.executionState858, after.executionState858, executionChanged ? 1u : 0u,
        static_cast<unsigned long long>(ToRva(caller)),
        static_cast<unsigned long long>(identity.selfVtableRva),
        identity.readableMask, identity.wrapper, identity.wrapperFirst,
        identity.wrapperFromPrimary ? "after" : "before", identity.callback,
        static_cast<unsigned long long>(identity.callbackVtableRva),
        static_cast<unsigned long long>(identity.owner),
        static_cast<unsigned long long>(identity.ownerVtableRva), identity.nodeId,
        stackText);
}

bool IsInSet(InstanceSet& set, void* value) {
    bool found = false;
    AcquireSRWLockShared(&set.lock);
    for (size_t index = 0; index < set.count; ++index) {
        if (set.items[index] == value) {
            found = true;
            break;
        }
    }
    ReleaseSRWLockShared(&set.lock);
    return found;
}

void AddToSet(InstanceSet& set, void* value) {
    if (!value) return;
    AcquireSRWLockExclusive(&set.lock);
    for (size_t index = 0; index < set.count; ++index) {
        if (set.items[index] == value) {
            ReleaseSRWLockExclusive(&set.lock);
            return;
        }
    }
    if (set.count < kMaxTrackedInstances) set.items[set.count++] = value;
    ReleaseSRWLockExclusive(&set.lock);
}

bool RemoveFromSet(InstanceSet& set, void* value) {
    if (!value) return false;
    bool removed = false;
    AcquireSRWLockExclusive(&set.lock);
    for (size_t index = 0; index < set.count; ++index) {
        if (set.items[index] != value) continue;
        set.items[index] = set.items[set.count - 1];
        set.items[set.count - 1] = nullptr;
        --set.count;
        removed = true;
        break;
    }
    ReleaseSRWLockExclusive(&set.lock);
    return removed;
}

bool ReadVtable(void* object, void*** outVtable) {
    if (!object || !outVtable) return false;
    __try {
        void** table = *reinterpret_cast<void***>(object);
        if (!table) return false;
        *outVtable = table;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

void ClassifyPointerWords(const uintptr_t* words, size_t count,
    uintptr_t* directRvas, uintptr_t* vtableRvas) {
    if (!words || !directRvas || !vtableRvas) return;
    for (size_t index = 0; index < count; ++index) {
        void* candidate = reinterpret_cast<void*>(words[index]);
        directRvas[index] = ToRva(candidate);
        void** table = nullptr;
        vtableRvas[index] = ReadVtable(candidate, &table) ? ToRva(table) : 0;
    }
}

void DumpVtable(const char* label, void* object) {
    void** table = nullptr;
    if (!ReadVtable(object, &table)) {
        Log("vtable %s object=%p unreadable", label, object);
        return;
    }
    Log("vtable %s object=%p table=%p table_rva=0x%llX", label, object, table,
        static_cast<unsigned long long>(ToRva(table)));
    __try {
        for (size_t index = 0; index < 32; ++index) {
            Log("  slot +0x%02llX -> %p (rva 0x%llX)",
                static_cast<unsigned long long>(index * sizeof(void*)), table[index],
                static_cast<unsigned long long>(ToRva(table[index])));
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("  vtable read stopped by access violation");
    }
}

bool ReadAsciiCandidate(const void* address, char* output, size_t capacity) {
    if (!address || !output || capacity < 2) return false;
    size_t length = 0;
    __try {
        const char* source = static_cast<const char*>(address);
        for (; length + 1 < capacity; ++length) {
            const unsigned char value = static_cast<unsigned char>(source[length]);
            if (!value) break;
            if (value < 0x20 || value > 0x7E) return false;
            output[length] = static_cast<char>(value);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
    output[length] = '\0';
    return length >= 4;
}

bool IsInterestingMenuText(const char* text) {
    return text && (strstr(text, "menu_") || strstr(text, ".muigraph") ||
        strstr(text, "helix/ui/graphs"));
}

bool IsContractsTerminalOnDemand(void* object) {
    if (!object) return false;
    const char* path = nullptr;
    if (!ReadDirectValue(static_cast<uint8_t*>(object) + 0x38, &path, sizeof(path)) ||
        !path) return false;
    char text[192] = {};
    return ReadAsciiCandidate(path, text, sizeof(text)) &&
        _stricmp(text, "helix/ui/graphs/menu_contracts_terminal.muigraph") == 0;
}

void CaptureContractsTerminalOnDemandState(void* object, void* context) {
    if (!g_capture.load(std::memory_order_relaxed)) return;
    OnDemandSnapshot snapshot = {};
    if (ReadDirectValue(static_cast<uint8_t*>(object) + 0x58,
            &snapshot.child58, sizeof(snapshot.child58))) snapshot.readableMask |= 1u << 0;
    if (ReadDirectValue(static_cast<uint8_t*>(object) + 0x60,
            &snapshot.owner60, sizeof(snapshot.owner60))) snapshot.readableMask |= 1u << 1;
    if (ReadDirectValue(static_cast<uint8_t*>(object) + 0x68,
            &snapshot.state68, sizeof(snapshot.state68))) snapshot.readableMask |= 1u << 2;
    if (ReadDirectValue(static_cast<uint8_t*>(object) + 0x6C,
            &snapshot.pushType6C, sizeof(snapshot.pushType6C))) snapshot.readableMask |= 1u << 3;
    if (!UpdateStateCache(g_menuGraphStackOnDemandStates, object,
            snapshot.readableMask, snapshot)) return;
    StateTrace("MenuGraphStackOnDemand.contracts.state", object, context,
        "mask=0x%X;child58=0x%llX;owner60=0x%llX;state68=%u;push_type6C=%u",
        snapshot.readableMask, static_cast<unsigned long long>(snapshot.child58),
        static_cast<unsigned long long>(snapshot.owner60), snapshot.state68,
        snapshot.pushType6C);
}

void LogInterestingMenuText(void* object, size_t fieldOffset, const char* source,
    const void* address) {
    char text[192] = {};
    if (ReadAsciiCandidate(address, text, sizeof(text)) && IsInterestingMenuText(text)) {
        Log("OnDemand text object=%p field=+0x%02llX source=%s address=%p value=%s",
            object, static_cast<unsigned long long>(fieldOffset), source, address, text);
    }
}

void DumpOnDemandObject(void* object) {
    if (!object) return;
    uintptr_t words[0x70 / sizeof(uintptr_t)] = {};
    __try {
        memcpy(words, object, sizeof(words));
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("OnDemand object=%p unreadable", object);
        return;
    }

    char line[2048] = {};
    int used = snprintf(line, sizeof(line), "OnDemand object=%p words", object);
    const size_t wordCount = sizeof(words) / sizeof(words[0]);
    for (size_t index = 0; index < wordCount && used > 0 &&
        static_cast<size_t>(used) < sizeof(line); ++index) {
        used += snprintf(line + used, sizeof(line) - static_cast<size_t>(used),
            " +%02llX=%p", static_cast<unsigned long long>(index * sizeof(uintptr_t)),
            reinterpret_cast<void*>(words[index]));
    }
    Log("%s", line);

    for (size_t index = 0; index < wordCount; ++index) {
        const size_t fieldOffset = index * sizeof(uintptr_t);
        const void* pointer = reinterpret_cast<const void*>(words[index]);
        LogInterestingMenuText(object, fieldOffset, "pointer", pointer);
    }
}

void RegisterInstance(NodeKind kind, void* object) {
    if (!object) {
        Log("factory %s returned null", KindName(kind));
        return;
    }
    AddToSet(SetFor(kind), object);
    void** table = nullptr;
    const uintptr_t tableRva = ReadVtable(object, &table) ? ToRva(table) : 0;
    Log("factory %s instance=%p vtable=%p vtable_rva=0x%llX", KindName(kind),
        object, table, static_cast<unsigned long long>(tableRva));
    Trace("factory", object, table, _ReturnAddress());

    if (kind == NodeKind::DamageDealt && table && g_cfg.hookDamageDealt) {
        __try {
            g_pendingDamageEval.store(table[kEvalVtableSlot], std::memory_order_release);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            Log("DamageDealt vtable slot +0x58 was unreadable");
        }
    }
}

void* __fastcall HookAimFactory() {
    void* object = g_origAimFactory();
    RegisterInstance(NodeKind::AimTarget, object);
    return object;
}
void* __fastcall HookDamageFactory() {
    void* object = g_origDamageFactory();
    RegisterInstance(NodeKind::DamageDealt, object);
    return object;
}
void* __fastcall HookHostileFactory() {
    void* object = g_origHostileFactory();
    RegisterInstance(NodeKind::IsHostile, object);
    return object;
}
void* __fastcall HookLinesFactory() {
    void* object = g_origLinesFactory();
    RegisterInstance(NodeKind::GraphicsLines, object);
    return object;
}
void* __fastcall HookMultiFactory() {
    void* object = g_origMultiFactory();
    RegisterInstance(NodeKind::GraphicsMultiSector, object);
    return object;
}
void* __fastcall HookInteractionListenerFactory() {
    void* object = g_origInteractionListenerFactory();
    RegisterInstance(NodeKind::InteractionListener, object);
    return object;
}
void* __fastcall HookInteractionModifierFactory() {
    void* object = g_origInteractionModifierFactory();
    RegisterInstance(NodeKind::InteractionModifier, object);
    return object;
}
void* __fastcall HookUIExecutionStateFactory() {
    void* object = g_origUIExecutionStateFactory();
    RegisterInstance(NodeKind::UIExecutionState, object);
    return object;
}
void* __fastcall HookMenuGraphStackOnDemandFactory() {
    void* object = g_origMenuGraphStackOnDemandFactory();
    RegisterInstance(NodeKind::MenuGraphStackOnDemand, object);
    return object;
}
void* __fastcall HookUISetExecutionStateFactory() {
    void* object = g_origUISetExecutionStateFactory();
    RegisterInstance(NodeKind::UISetExecutionState, object);
    return object;
}

void* __fastcall HookSequenceLevelEntityToArchive(void* component, void* archive) {
    void* caller = _ReturnAddress();
    void* result = g_origSequenceLevelEntityToArchive(component, archive);
    g_sequenceLevelEntityToArchiveCalls.fetch_add(1, std::memory_order_relaxed);
    CaptureSequenceLevelEntityUid("to_archive", component, archive, caller);
    return result;
}

void* __fastcall HookSequenceLevelEntityFromArchive(void* component, void* archive) {
    void* caller = _ReturnAddress();
    void* result = g_origSequenceLevelEntityFromArchive(component, archive);
    g_sequenceLevelEntityFromArchiveCalls.fetch_add(1, std::memory_order_relaxed);
    CaptureSequenceLevelEntityUid("from_archive", component, archive, caller);
    return result;
}

bool __fastcall HookSequenceBlockCollectionLoad(void* sequenceBlock, void* archive) {
    void* caller = _ReturnAddress();
    uint64_t generationBefore = 0;
    ReadCouchThreadMarker(&generationBefore, nullptr);
    const bool result = g_origSequenceBlockCollectionLoad(sequenceBlock, archive);
    uint64_t generationAfter = 0;
    void* component = nullptr;
    ReadCouchThreadMarker(&generationAfter, &component);
    if (generationAfter != generationBefore) {
        DumpCouchCollectionLoad(sequenceBlock, archive, component, result, caller,
            generationAfter - generationBefore);
    }
    return result;
}

bool __fastcall HookTargetTrackFromArchive(void* sequenceBlock, void* archive,
    void* context) {
    void* caller = _ReturnAddress();
    uint8_t blockBefore[0x200] = {};
    uint8_t contextBefore[0x100] = {};
    const bool blockBeforeReadable = sequenceBlock &&
        ReadDirectValue(sequenceBlock, blockBefore, sizeof(blockBefore));
    const bool contextBeforeReadable = context &&
        ReadDirectValue(context, contextBefore, sizeof(contextBefore));
    uint64_t generationBefore = 0;
    ReadCouchThreadMarker(&generationBefore, nullptr);
    const bool result = g_origTargetTrackFromArchive(sequenceBlock, archive, context);
    uint64_t generationAfter = 0;
    void* component = nullptr;
    ReadCouchThreadMarker(&generationAfter, &component);
    if (generationAfter != generationBefore) {
        DumpCouchTargetTrackLoad(sequenceBlock, archive, context, component, result,
            caller, generationAfter - generationBefore, blockBefore,
            blockBeforeReadable, contextBefore, contextBeforeReadable);
    }
    return result;
}

__declspec(noinline) bool __fastcall HookNixBowlInnerEvaluator(void* layout,
    void* runtime, uint8_t mode, int32_t dispatchIndex, int32_t requestedIndex,
    uint8_t allowFallback) {
    void* caller = _ReturnAddress();
    const uintptr_t callerRva = ToRva(caller);
    const bool gated = g_nixBowlDispatchDepth != 0 &&
        g_capture.load(std::memory_order_relaxed);
    if (!gated) {
        return g_origNixBowlInnerEvaluator(layout, runtime, mode, dispatchIndex,
            requestedIndex, allowFallback);
    }
    ++g_nixBowlInnerSeen;
    g_nixBowlInnerLastCallerRva = callerRva;
    const bool expectedCaller = callerRva == kRvaNixBowlInnerEvaluatorReturn;
    if (!expectedCaller)
        g_nixBowlInnerCallerMismatches.fetch_add(1, std::memory_order_relaxed);

    uintptr_t layoutWordsBefore[8] = {};
    uintptr_t layoutWordsAfter[8] = {};
    uintptr_t layoutWordRvas[8] = {};
    uintptr_t layoutVtableRvas[8] = {};
    uint8_t runtimeFlagsBefore = 0;
    uint8_t runtimeFlagsAfter = 0;
    uint32_t runtimeCountBefore = 0;
    uint32_t runtimeCountAfter = 0;
    uint32_t runtimeStateBefore = 0;
    uint32_t runtimeStateAfter = 0;

    const bool layoutBeforeReadable = layout &&
        ReadDirectValue(layout, layoutWordsBefore, sizeof(layoutWordsBefore));
    if (layoutBeforeReadable) {
        ClassifyPointerWords(layoutWordsBefore, 8, layoutWordRvas,
            layoutVtableRvas);
    }
    const bool flagsBeforeReadable = runtime && ReadDirectValue(
        static_cast<uint8_t*>(runtime) + 0x5D, &runtimeFlagsBefore,
        sizeof(runtimeFlagsBefore));
    const bool countBeforeReadable = runtime && ReadDirectValue(
        static_cast<uint8_t*>(runtime) + 0x4C, &runtimeCountBefore,
        sizeof(runtimeCountBefore));
    const bool stateBeforeReadable = runtime && ReadDirectValue(
        static_cast<uint8_t*>(runtime) + 0x58, &runtimeStateBefore,
        sizeof(runtimeStateBefore));

    const bool result = g_origNixBowlInnerEvaluator(layout, runtime, mode,
        dispatchIndex, requestedIndex, allowFallback);

    const bool layoutAfterReadable = layout &&
        ReadDirectValue(layout, layoutWordsAfter, sizeof(layoutWordsAfter));
    const bool flagsAfterReadable = runtime && ReadDirectValue(
        static_cast<uint8_t*>(runtime) + 0x5D, &runtimeFlagsAfter,
        sizeof(runtimeFlagsAfter));
    const bool countAfterReadable = runtime && ReadDirectValue(
        static_cast<uint8_t*>(runtime) + 0x4C, &runtimeCountAfter,
        sizeof(runtimeCountAfter));
    const bool stateAfterReadable = runtime && ReadDirectValue(
        static_cast<uint8_t*>(runtime) + 0x58, &runtimeStateAfter,
        sizeof(runtimeStateAfter));
    uint32_t layoutChangedMask = 0;
    if (layoutBeforeReadable && layoutAfterReadable) {
        for (size_t index = 0; index < 8; ++index) {
            if (layoutWordsBefore[index] != layoutWordsAfter[index])
                layoutChangedMask |= 1u << index;
        }
    }

    const bool open = g_nixBowlDispatchOpen;
    if (open) g_nixBowlInnerOpenCalls.fetch_add(1, std::memory_order_relaxed);
    else g_nixBowlInnerCloseCalls.fetch_add(1, std::memory_order_relaxed);
    const char* eventName = open ? "NixBowl.inner_eval.open" :
        "NixBowl.inner_eval.close";
    void* expectedLayout = g_nixBowlSelectedEntry ?
        static_cast<uint8_t*>(g_nixBowlSelectedEntry) + 0x18 : nullptr;
    Trace(eventName, layout, runtime, caller);
    StateTrace(eventName, layout, runtime,
        "outer_selector=%u;mode_r8=%u;dispatch_index_r9=%d;requested_index=%d;"
        "allow_fallback=%u;caller_rva=0x%llX;expected_caller=%u;selected_entry=%p;"
        "expected_layout=%p;layout_match=%u;layout_before_readable=%u;"
        "layout_after_readable=%u;layout_changed_mask=0x%X;"
        "layout_words=%llX,%llX,%llX,%llX,%llX,%llX,%llX,%llX;"
        "layout_rvas=%llX,%llX,%llX,%llX,%llX,%llX,%llX,%llX;"
        "layout_vtable_rvas=%llX,%llX,%llX,%llX,%llX,%llX,%llX,%llX;"
        "runtime_flags=%u,%u;runtime_count=%u,%u;runtime_state=%u,%u;"
        "runtime_readable=%u,%u,%u,%u,%u,%u;result=%u",
        g_nixBowlDispatchSelector, mode, dispatchIndex, requestedIndex,
        allowFallback, static_cast<unsigned long long>(callerRva),
        expectedCaller ? 1u : 0u,
        g_nixBowlSelectedEntry, expectedLayout, layout == expectedLayout ? 1u : 0u,
        layoutBeforeReadable ? 1u : 0u, layoutAfterReadable ? 1u : 0u,
        layoutChangedMask,
        static_cast<unsigned long long>(layoutWordsBefore[0]),
        static_cast<unsigned long long>(layoutWordsBefore[1]),
        static_cast<unsigned long long>(layoutWordsBefore[2]),
        static_cast<unsigned long long>(layoutWordsBefore[3]),
        static_cast<unsigned long long>(layoutWordsBefore[4]),
        static_cast<unsigned long long>(layoutWordsBefore[5]),
        static_cast<unsigned long long>(layoutWordsBefore[6]),
        static_cast<unsigned long long>(layoutWordsBefore[7]),
        static_cast<unsigned long long>(layoutWordRvas[0]),
        static_cast<unsigned long long>(layoutWordRvas[1]),
        static_cast<unsigned long long>(layoutWordRvas[2]),
        static_cast<unsigned long long>(layoutWordRvas[3]),
        static_cast<unsigned long long>(layoutWordRvas[4]),
        static_cast<unsigned long long>(layoutWordRvas[5]),
        static_cast<unsigned long long>(layoutWordRvas[6]),
        static_cast<unsigned long long>(layoutWordRvas[7]),
        static_cast<unsigned long long>(layoutVtableRvas[0]),
        static_cast<unsigned long long>(layoutVtableRvas[1]),
        static_cast<unsigned long long>(layoutVtableRvas[2]),
        static_cast<unsigned long long>(layoutVtableRvas[3]),
        static_cast<unsigned long long>(layoutVtableRvas[4]),
        static_cast<unsigned long long>(layoutVtableRvas[5]),
        static_cast<unsigned long long>(layoutVtableRvas[6]),
        static_cast<unsigned long long>(layoutVtableRvas[7]),
        runtimeFlagsBefore, runtimeFlagsAfter, runtimeCountBefore,
        runtimeCountAfter, runtimeStateBefore, runtimeStateAfter,
        flagsBeforeReadable ? 1u : 0u, flagsAfterReadable ? 1u : 0u,
        countBeforeReadable ? 1u : 0u, countAfterReadable ? 1u : 0u,
        stateBeforeReadable ? 1u : 0u, stateAfterReadable ? 1u : 0u,
        result ? 1u : 0u);
    return result;
}

__declspec(noinline) bool __fastcall HookNixBowlGraphDispatch(void* branchObject,
    void* evaluationContext, uint8_t selector) {
    void* caller = _ReturnAddress();
    const uintptr_t callerRva = ToRva(caller);
    const bool open = callerRva == kRvaNixBowlOpenReturn;
    const bool close = callerRva == kRvaNixBowlCloseReturn;
    const bool capturing = g_capture.load(std::memory_order_relaxed);
    if ((!open && !close) || !capturing) {
        return g_origNixBowlGraphDispatch(branchObject, evaluationContext, selector);
    }

    uint32_t readableMask = 0;
    void* rawSelected = nullptr;
    void* selectedGraph = nullptr;
    void* runtimeObject = nullptr;
    uint32_t selectedFlags = 0;
    uint16_t executionStateBefore = 0;
    uint16_t executionStateAfter = 0;
    uint8_t sourceSelector = 0;
    uintptr_t slotWordsBefore[4] = {};
    uintptr_t slotWordsAfter[4] = {};
    uintptr_t evaluatorWordsBefore[4] = {};
    uintptr_t evaluatorWordsAfter[4] = {};
    uintptr_t slotWordRvas[4] = {};
    uintptr_t slotVtableRvas[4] = {};
    uintptr_t evaluatorWordRvas[4] = {};
    uintptr_t evaluatorVtableRvas[4] = {};

    if (evaluationContext && ReadDirectValue(
            static_cast<uint8_t*>(evaluationContext) + 0x20 +
                static_cast<size_t>(selector) * sizeof(void*),
            &rawSelected, sizeof(rawSelected))) {
        readableMask |= 1u << 0;
    }
    if (rawSelected && ReadDirectValue(static_cast<uint8_t*>(rawSelected) + 0x1C,
            &selectedFlags, sizeof(selectedFlags))) {
        readableMask |= 1u << 1;
        if ((selectedFlags & 0x10000000u) != 0) {
            if (ReadDirectValue(rawSelected, &selectedGraph, sizeof(selectedGraph)))
                readableMask |= 1u << 2;
        } else {
            selectedGraph = rawSelected;
            readableMask |= 1u << 2;
        }
    }
    if (evaluationContext && ReadDirectValue(
            static_cast<uint8_t*>(evaluationContext) + 0x828,
            &runtimeObject, sizeof(runtimeObject))) {
        readableMask |= 1u << 3;
    }
    if (evaluationContext && ReadDirectValue(
            static_cast<uint8_t*>(evaluationContext) + 0x858,
            &executionStateBefore, sizeof(executionStateBefore))) {
        readableMask |= 1u << 4;
    }
    const size_t selectorOffset = open ? 0x102 : 0x106;
    if (branchObject && ReadDirectValue(
            static_cast<uint8_t*>(branchObject) + selectorOffset,
            &sourceSelector, sizeof(sourceSelector))) {
        readableMask |= 1u << 5;
    }
    const bool slotBeforeReadable = selectedGraph &&
        ReadDirectValue(selectedGraph, slotWordsBefore, sizeof(slotWordsBefore));
    const bool evaluatorBeforeReadable = selectedGraph &&
        ReadDirectValue(static_cast<uint8_t*>(selectedGraph) + 0x18,
            evaluatorWordsBefore, sizeof(evaluatorWordsBefore));
    if (slotBeforeReadable) {
        readableMask |= 1u << 7;
        ClassifyPointerWords(slotWordsBefore, 4, slotWordRvas, slotVtableRvas);
    }
    if (evaluatorBeforeReadable) {
        readableMask |= 1u << 8;
        ClassifyPointerWords(evaluatorWordsBefore, 4, evaluatorWordRvas,
            evaluatorVtableRvas);
    }

    const uint32_t previousDepth = g_nixBowlDispatchDepth;
    const bool previousOpen = g_nixBowlDispatchOpen;
    const uint8_t previousSelector = g_nixBowlDispatchSelector;
    void* previousSelectedEntry = g_nixBowlSelectedEntry;
    g_nixBowlDispatchDepth = previousDepth + 1;
    g_nixBowlDispatchOpen = open;
    g_nixBowlDispatchSelector = selector;
    g_nixBowlSelectedEntry = selectedGraph;
    const uint32_t innerSeenBefore = g_nixBowlInnerSeen;
    const uintptr_t previousInnerLastCallerRva = g_nixBowlInnerLastCallerRva;
    g_nixBowlInnerLastCallerRva = 0;
    const bool result =
        g_origNixBowlGraphDispatch(branchObject, evaluationContext, selector);
    const uint32_t innerCalls = g_nixBowlInnerSeen - innerSeenBefore;
    const uintptr_t innerLastCallerRva = g_nixBowlInnerLastCallerRva;
    g_nixBowlInnerLastCallerRva = previousInnerLastCallerRva;
    g_nixBowlDispatchDepth = previousDepth;
    g_nixBowlDispatchOpen = previousOpen;
    g_nixBowlDispatchSelector = previousSelector;
    g_nixBowlSelectedEntry = previousSelectedEntry;

    if (evaluationContext && ReadDirectValue(
            static_cast<uint8_t*>(evaluationContext) + 0x858,
            &executionStateAfter, sizeof(executionStateAfter))) {
        readableMask |= 1u << 6;
    }
    const bool slotAfterReadable = selectedGraph &&
        ReadDirectValue(selectedGraph, slotWordsAfter, sizeof(slotWordsAfter));
    const bool evaluatorAfterReadable = selectedGraph &&
        ReadDirectValue(static_cast<uint8_t*>(selectedGraph) + 0x18,
            evaluatorWordsAfter, sizeof(evaluatorWordsAfter));
    const bool slotChanged = slotBeforeReadable && slotAfterReadable &&
        memcmp(slotWordsBefore, slotWordsAfter, sizeof(slotWordsBefore)) != 0;
    const bool evaluatorChanged = evaluatorBeforeReadable && evaluatorAfterReadable &&
        memcmp(evaluatorWordsBefore, evaluatorWordsAfter,
            sizeof(evaluatorWordsBefore)) != 0;
    if (open) g_nixBowlGraphOpenCalls.fetch_add(1, std::memory_order_relaxed);
    else g_nixBowlGraphCloseCalls.fetch_add(1, std::memory_order_relaxed);

    const char* eventName = open ? "NixBowl.graph_dispatch.open" :
        "NixBowl.graph_dispatch.close";
    Trace(eventName, branchObject, evaluationContext, caller);
    StateTrace(eventName, branchObject, evaluationContext,
        "mask=0x%X;selector=%u;source_selector=%u;selector_offset=0x%llX;"
        "raw_selected=%p;selected_flags=0x%08X;selected_graph=%p;"
        "runtime828=%p;execution_before858=%u;execution_after858=%u;"
        "result=%u;caller_rva=0x%llX;inner_calls=%u;inner_last_caller_rva=0x%llX;"
        "slot_after_readable=%u;slot_changed=%u;"
        "slot_words=%llX,%llX,%llX,%llX;slot_rvas=%llX,%llX,%llX,%llX;"
        "slot_vtable_rvas=%llX,%llX,%llX,%llX;"
        "evaluator_after_readable=%u;evaluator_changed=%u;"
        "evaluator_words=%llX,%llX,%llX,%llX;"
        "evaluator_rvas=%llX,%llX,%llX,%llX;"
        "evaluator_vtable_rvas=%llX,%llX,%llX,%llX",
        readableMask, selector, sourceSelector,
        static_cast<unsigned long long>(selectorOffset), rawSelected, selectedFlags,
        selectedGraph, runtimeObject, executionStateBefore, executionStateAfter,
        result ? 1u : 0u, static_cast<unsigned long long>(callerRva), innerCalls,
        static_cast<unsigned long long>(innerLastCallerRva),
        slotAfterReadable ? 1u : 0u, slotChanged ? 1u : 0u,
        static_cast<unsigned long long>(slotWordsBefore[0]),
        static_cast<unsigned long long>(slotWordsBefore[1]),
        static_cast<unsigned long long>(slotWordsBefore[2]),
        static_cast<unsigned long long>(slotWordsBefore[3]),
        static_cast<unsigned long long>(slotWordRvas[0]),
        static_cast<unsigned long long>(slotWordRvas[1]),
        static_cast<unsigned long long>(slotWordRvas[2]),
        static_cast<unsigned long long>(slotWordRvas[3]),
        static_cast<unsigned long long>(slotVtableRvas[0]),
        static_cast<unsigned long long>(slotVtableRvas[1]),
        static_cast<unsigned long long>(slotVtableRvas[2]),
        static_cast<unsigned long long>(slotVtableRvas[3]),
        evaluatorAfterReadable ? 1u : 0u, evaluatorChanged ? 1u : 0u,
        static_cast<unsigned long long>(evaluatorWordsBefore[0]),
        static_cast<unsigned long long>(evaluatorWordsBefore[1]),
        static_cast<unsigned long long>(evaluatorWordsBefore[2]),
        static_cast<unsigned long long>(evaluatorWordsBefore[3]),
        static_cast<unsigned long long>(evaluatorWordRvas[0]),
        static_cast<unsigned long long>(evaluatorWordRvas[1]),
        static_cast<unsigned long long>(evaluatorWordRvas[2]),
        static_cast<unsigned long long>(evaluatorWordRvas[3]),
        static_cast<unsigned long long>(evaluatorVtableRvas[0]),
        static_cast<unsigned long long>(evaluatorVtableRvas[1]),
        static_cast<unsigned long long>(evaluatorVtableRvas[2]),
        static_cast<unsigned long long>(evaluatorVtableRvas[3]));
    return result;
}

void __fastcall HookAimEval(void* self, void* context) {
    if (g_cfg.rawEvaluatorCalls) Trace("PlayerAimTarget.eval", self, context, _ReturnAddress());
    g_origAimEval(self, context);
    CaptureAimState(self, context);
}
void __fastcall HookHostileEval(void* self, void* context) {
    if (g_cfg.rawEvaluatorCalls) Trace("IsHostile.eval", self, context, _ReturnAddress());
    g_origHostileEval(self, context);
    CaptureHostileState(self, context);
}
void __fastcall HookGraphicsEval(void* self, void* context) {
    const bool lines = IsInSet(g_linesInstances, self);
    const bool multi = !lines && IsInSet(g_multiInstances, self);
    if (g_cfg.rawEvaluatorCalls && lines) Trace("GraphicsLines.eval", self, context, _ReturnAddress());
    else if (g_cfg.rawEvaluatorCalls && multi) Trace("GraphicsMultiSector.eval", self, context, _ReturnAddress());
    g_origGraphicsEval(self, context);
    if (lines || multi) CaptureGraphicsState(self, context, lines);
}
void __fastcall HookDamageEval(void* self, void* context) {
    if (g_cfg.rawEvaluatorCalls) Trace("DamageDealt.eval", self, context, _ReturnAddress());
    g_insideDamageEval = true;
    g_origDamageEval(self, context);
    g_insideDamageEval = false;
    CaptureDamageState(self, context);
}
void __fastcall HookInteractionListenerEval(void* self, void* context) {
    const bool capturing = g_capture.load(std::memory_order_relaxed);
    InteractionSnapshot before = {};
    if (g_cfg.interactionTransitionDetails && capturing) {
        ReadInteractionSnapshot(context, &before);
    }
    void* caller = _ReturnAddress();
    if (g_cfg.interactionEvaluatorCalls)
        Trace("InteractionListener.eval", self, context, caller);
    g_origInteractionListenerEval(self, context);
    if (capturing) {
        InteractionSnapshot after = {};
        ReadInteractionSnapshot(context, &after);
        CaptureInteractionTransition(self, context, NodeKind::InteractionListener,
            before, after, caller);
        CaptureInteractionState(self, context, NodeKind::InteractionListener, &after,
            caller);
    }
}
void __fastcall HookInteractionModifierEval(void* self, void* context) {
    const bool capturing = g_capture.load(std::memory_order_relaxed);
    InteractionSnapshot before = {};
    if (g_cfg.interactionTransitionDetails && capturing) {
        ReadInteractionSnapshot(context, &before);
    }
    void* caller = _ReturnAddress();
    if (g_cfg.interactionEvaluatorCalls)
        Trace("InteractionModifier.eval", self, context, caller);
    g_origInteractionModifierEval(self, context);
    if (capturing) {
        InteractionSnapshot after = {};
        ReadInteractionSnapshot(context, &after);
        CaptureInteractionTransition(self, context, NodeKind::InteractionModifier,
            before, after, caller);
        CaptureInteractionState(self, context, NodeKind::InteractionModifier, &after,
            caller);
    }
}
void __fastcall HookUIExecutionStateEval(void* self, void* context) {
    if (g_cfg.interactionEvaluatorCalls)
        Trace("UIExecutionState.eval", self, context, _ReturnAddress());
    g_origUIExecutionStateEval(self, context);
    CaptureInteractionState(self, context, NodeKind::UIExecutionState);
}

void __fastcall HookMenuGraphStackOnDemandEval(void* self, void* context) {
    const bool contractsTerminal = IsContractsTerminalOnDemand(self);
    if (contractsTerminal && g_capture.load(std::memory_order_relaxed)) {
        Trace("MenuGraphStackOnDemand.contracts.eval", self, context, _ReturnAddress());
        ArmUISetExecutionWindow(self, context);
    }
    g_origMenuGraphStackOnDemandEval(self, context);
    if (contractsTerminal) CaptureContractsTerminalOnDemandState(self, context);
}

void __fastcall HookUISetExecutionStateEval(void* self, void* context) {
    void* caller = _ReturnAddress();
    CaptureUISetExecutionStateCall(self, context, caller);
    g_origUISetExecutionStateEval(self, context);
}

void __fastcall HookUIExecutionStateManagerSet(void* manager, void* stateObject) {
    void* caller = _ReturnAddress();
    uint32_t stateId = 0xFFFFFFFFu;
    const bool readable = stateObject && ReadDirectValue(
        static_cast<uint8_t*>(stateObject) + 0x08, &stateId, sizeof(stateId));
    const bool targetState = readable &&
        (stateId == kContractsTerminalStateId || stateId == kHideLocalPlayerStateId);
    if (targetState && g_capture.load(std::memory_order_relaxed) &&
        g_contractsSetExecutionWindow.load(std::memory_order_relaxed)) {
        SelectContractsStateManager(manager, stateId, caller);
    }
    SampleContractsStateManager("set_pre", manager, stateId, caller);
    g_origUIExecutionStateManagerSet(manager, stateObject);
    SampleContractsStateManager("set_post", manager, stateId, caller);
}

void __fastcall HookUIExecutionStateBoolCallback(void* base, const void* closure,
    const void* eventValue) {
    uintptr_t target = 0;
    int32_t thisAdjustment = 0;
    void* valueStorage = nullptr;
    uint8_t value = 0;
    uint32_t readableMask = 0;
    if (closure && ReadDirectValue(closure, &target, sizeof(target))) readableMask |= 1u << 0;
    if (closure && ReadDirectValue(reinterpret_cast<const uint8_t*>(closure) + 0x08,
            &thisAdjustment, sizeof(thisAdjustment))) readableMask |= 1u << 1;
    if (eventValue && ReadDirectValue(eventValue, &valueStorage, sizeof(valueStorage)) &&
        valueStorage) {
        readableMask |= 1u << 2;
        if (ReadDirectValue(valueStorage, &value, sizeof(value))) readableMask |= 1u << 3;
    }
    void* adjustedSelf = base;
    if (base && (readableMask & (1u << 1))) {
        adjustedSelf = reinterpret_cast<uint8_t*>(base) + thisAdjustment;
    }
    Trace("UIExecutionState.bool_callback", adjustedSelf, valueStorage, _ReturnAddress());
    StateTrace("UIExecutionState.bool_callback", adjustedSelf, valueStorage,
        "mask=0x%X;base=%p;closure=%p;target=%p;target_rva=0x%llX;this_adjustment=%d;value=%u",
        readableMask, base, closure, reinterpret_cast<void*>(target),
        static_cast<unsigned long long>(ToRva(reinterpret_cast<void*>(target))),
        thisAdjustment, value);
    if ((readableMask & 0xFu) == 0xFu && target && adjustedSelf) {
        reinterpret_cast<UIExecutionStateBoolTargetFn>(target)(adjustedSelf, value);
    }
}

void __fastcall HookInteractionListenerSetup(void* self, void* context) {
    g_origInteractionListenerSetup(self, context);
    if (!context) return;

    void* wrapper = nullptr;
    void* callback = nullptr;
    void** table = nullptr;
    uint64_t owner = 0;
    uint32_t nodeId = 0;
    if (!ReadDirectValue(reinterpret_cast<uint8_t*>(context) + 0x840,
            &wrapper, sizeof(wrapper)) || !wrapper ||
        !ReadDirectValue(wrapper, &callback, sizeof(callback)) || !callback ||
        !ReadVtable(callback, &table) ||
        ToRva(table) != kRvaVtableInteractionCallback) {
        return;
    }
    const bool known = IsInSet(g_interactionCallbackInstances, callback);
    AddToSet(g_interactionCallbackInstances, callback);
    if (known) return;

    ReadDirectValue(reinterpret_cast<uint8_t*>(callback) + 0x10, &owner, sizeof(owner));
    ReadDirectValue(reinterpret_cast<uint8_t*>(callback) + 0x18, &nodeId, sizeof(nodeId));
    Log("interaction callback registered callback=%p listener=%p context=%p wrapper=%p owner=%p node_id=%u",
        callback, self, context, wrapper, reinterpret_cast<void*>(owner), nodeId);
    Trace("InteractionListener.callback_register", callback, self, _ReturnAddress());
    StateTrace("InteractionListener.callback_register", callback, context,
        "listener=%p;wrapper=%p;owner=0x%llX;node_id=%u", self, wrapper,
        static_cast<unsigned long long>(owner), nodeId);
}

bool __fastcall HookInteractionCallbackSignal(void* self, volatile LONG* counter) {
    LONG before = 0;
    if (counter) ReadDirectValue(const_cast<const LONG*>(counter), &before, sizeof(before));
    const bool result = g_origInteractionCallbackSignal(self, counter);
    LONG after = 0;
    if (counter) ReadDirectValue(const_cast<const LONG*>(counter), &after, sizeof(after));
    uint64_t owner = 0;
    uint32_t nodeId = 0;
    ReadDirectValue(reinterpret_cast<uint8_t*>(self) + 0x10, &owner, sizeof(owner));
    ReadDirectValue(reinterpret_cast<uint8_t*>(self) + 0x18, &nodeId, sizeof(nodeId));
    g_interactionCallbackSignals.fetch_add(1, std::memory_order_relaxed);
    Trace("InteractionListener.callback_signal", self, reinterpret_cast<void*>(owner),
        _ReturnAddress());
    StateTrace("InteractionListener.callback_signal", self, reinterpret_cast<void*>(owner),
        "node_id=%u;counter=%p;before=%ld;after=%ld;completed=%u", nodeId,
        const_cast<LONG*>(counter), before, after, result ? 1u : 0u);
    return result;
}

void* __fastcall HookInteractionCallbackDestroy(void* self, uint32_t flags) {
    const bool tracked = RemoveFromSet(g_interactionCallbackInstances, self);
    if (tracked) {
        g_interactionCallbackDestroys.fetch_add(1, std::memory_order_relaxed);
        Trace("InteractionListener.callback_destroy", self, nullptr, _ReturnAddress());
        StateTrace("InteractionListener.callback_destroy", self, nullptr, "flags=0x%X", flags);
    }
    return g_origInteractionCallbackDestroy(self, flags);
}

bool MatchBytes(uintptr_t rva, const uint8_t* expected, size_t length) {
    if (rva + length > g_imageSize) return false;
    __try {
        return memcmp(reinterpret_cast<const void*>(g_base + rva), expected, length) == 0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool ValidateBuild() {
    auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(g_base);
    auto* nt = reinterpret_cast<IMAGE_NT_HEADERS64*>(g_base + dos->e_lfanew);
    WIN32_FILE_ATTRIBUTE_DATA data = {};
    char exePath[MAX_PATH] = {};
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    GetFileAttributesExA(exePath, GetFileExInfoStandard, &data);
    const ULONGLONG fileSize = (static_cast<ULONGLONG>(data.nFileSizeHigh) << 32) |
        data.nFileSizeLow;
    Log("build timestamp=0x%08lX image_size=0x%08lX file_size=%llu base=%p",
        nt->FileHeader.TimeDateStamp, nt->OptionalHeader.SizeOfImage,
        static_cast<unsigned long long>(fileSize), reinterpret_cast<void*>(g_base));
    const bool match = nt->FileHeader.TimeDateStamp == kExpectedTimeDateStamp &&
        nt->OptionalHeader.SizeOfImage == kExpectedSizeOfImage &&
        fileSize == kExpectedFileSize;
    if (!match) Log("build does not match the analyzed Outlaws.exe profile");
    return match;
}

bool CreateHook(const char* name, uintptr_t rva, void* detour, void** original,
    const uint8_t* prolog, size_t prologLength) {
    if (!MatchBytes(rva, prolog, prologLength)) {
        Log("SKIP hook %-32s prolog mismatch at rva 0x%llX", name,
            static_cast<unsigned long long>(rva));
        return false;
    }
    void* target = reinterpret_cast<void*>(g_base + rva);
    const MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status != MH_OK) {
        Log("FAIL hook %-32s rva=0x%llX status=%s", name,
            static_cast<unsigned long long>(rva), MH_StatusToString(status));
        return false;
    }
    const MH_STATUS enable = MH_EnableHook(target);
    if (enable != MH_OK) {
        Log("FAIL enable %-30s status=%s", name, MH_StatusToString(enable));
        MH_RemoveHook(target);
        return false;
    }
    Log("hooked %-35s rva=0x%llX", name, static_cast<unsigned long long>(rva));
    return true;
}

bool PatchUIExecutionStateBoolCallback() {
    if (!g_cfg.hookUIExecutionState || g_uiExecutionStateBoolCallbackPatched) return true;
    static const uint8_t expected[] = {
        0xC5,0xF8,0x10,0x02,0x49,0x8B,0x10,0x4C,
        0x8B,0xC9,0xC4,0xE3,0x79,0x16,0xC0,0x02,
    };
    if (!MatchBytes(kRvaUIExecutionStateBoolCallback, expected, sizeof(expected))) {
        Log("SKIP callback UIExecutionState bool patch: exact bytes did not match");
        return false;
    }

    auto* target = reinterpret_cast<uint8_t*>(g_base + kRvaUIExecutionStateBoolCallback);
    uint8_t replacement[sizeof(expected)] = {
        0xFF,0x25,0x00,0x00,0x00,0x00,
        0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
        0x90,0x90,
    };
    const uintptr_t detour = reinterpret_cast<uintptr_t>(&HookUIExecutionStateBoolCallback);
    memcpy(replacement + 6, &detour, sizeof(detour));

    DWORD oldProtect = 0;
    if (!VirtualProtect(target, sizeof(replacement), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        Log("FAIL callback UIExecutionState bool patch: VirtualProtect error=%lu", GetLastError());
        return false;
    }
    memcpy(target, replacement, sizeof(replacement));
    FlushInstructionCache(GetCurrentProcess(), target, sizeof(replacement));
    DWORD restoredProtect = 0;
    VirtualProtect(target, sizeof(replacement), oldProtect, &restoredProtect);
    g_uiExecutionStateBoolCallbackPatched = true;
    Log("patched callback UIExecutionState bool rva=0x%llX exact_len=%llu",
        static_cast<unsigned long long>(kRvaUIExecutionStateBoolCallback),
        static_cast<unsigned long long>(sizeof(replacement)));
    return true;
}

bool PatchInteractionCallbackVtable() {
    if (!g_cfg.hookInteractionCallbacks || g_interactionCallbackVtablePatched) return true;
    void** slot = reinterpret_cast<void**>(g_base + kRvaVtableInteractionCallback) + 1;
    void* expected = reinterpret_cast<void*>(g_base + kRvaInteractionCallbackSignal);
    void* current = nullptr;
    __try {
        current = *slot;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        Log("SKIP interaction callback signal: vtable slot was unreadable");
        return false;
    }
    if (current != expected) {
        Log("SKIP interaction callback signal: vtable +0x08 expected=%p actual=%p",
            expected, current);
        return false;
    }

    DWORD oldProtect = 0;
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &oldProtect)) {
        Log("FAIL interaction callback signal: VirtualProtect error=%lu", GetLastError());
        return false;
    }
    g_origInteractionCallbackSignal =
        reinterpret_cast<InteractionCallbackSignalFn>(current);
    InterlockedExchangePointer(reinterpret_cast<PVOID volatile*>(slot),
        reinterpret_cast<void*>(&HookInteractionCallbackSignal));
    DWORD restoredProtect = 0;
    VirtualProtect(slot, sizeof(void*), oldProtect, &restoredProtect);
    g_interactionCallbackVtablePatched = true;
    Log("patched InteractionListener callback vtable +0x08 table_rva=0x%llX original_rva=0x%llX",
        static_cast<unsigned long long>(kRvaVtableInteractionCallback),
        static_cast<unsigned long long>(kRvaInteractionCallbackSignal));
    return true;
}

bool CreateConsumerHook(const char* name, uintptr_t rva, void* detour, void** original,
    const uint8_t* prolog, size_t prologLength) {
    if (g_consumerHookCount >= sizeof(g_consumerHookTargets) / sizeof(g_consumerHookTargets[0]))
        return false;
    if (!MatchBytes(rva, prolog, prologLength)) {
        Log("SKIP hook %-32s prolog mismatch at rva 0x%llX", name,
            static_cast<unsigned long long>(rva));
        return false;
    }
    void* target = reinterpret_cast<void*>(g_base + rva);
    const MH_STATUS status = MH_CreateHook(target, detour, original);
    if (status != MH_OK) {
        Log("FAIL create dormant %-22s rva=0x%llX status=%s", name,
            static_cast<unsigned long long>(rva), MH_StatusToString(status));
        return false;
    }
    g_consumerHookTargets[g_consumerHookCount++] = target;
    Log("created dormant %-26s rva=0x%llX", name, static_cast<unsigned long long>(rva));
    return true;
}

void SetConsumerHooksEnabled(bool enabled) {
    if (!g_cfg.consumerTrace || !g_consumerHookCount || enabled == g_consumerHooksEnabled) return;
    size_t queued = 0;
    for (size_t index = 0; index < g_consumerHookCount; ++index) {
        const MH_STATUS status = enabled ? MH_QueueEnableHook(g_consumerHookTargets[index]) :
            MH_QueueDisableHook(g_consumerHookTargets[index]);
        if (status == MH_OK) ++queued;
        else Log("consumer hook queue %s failed target=%p status=%s",
            enabled ? "enable" : "disable", g_consumerHookTargets[index],
            MH_StatusToString(status));
    }
    if (!queued) return;
    const MH_STATUS applied = MH_ApplyQueued();
    if (applied == MH_OK) {
        g_consumerHooksEnabled = enabled;
        Log("consumer storage hooks %s (%llu targets)", enabled ? "enabled" : "disabled",
            static_cast<unsigned long long>(queued));
    } else {
        Log("consumer hook apply %s failed: %s", enabled ? "enable" : "disable",
            MH_StatusToString(applied));
    }
}

void InstallKnownHooks() {
    static const uint8_t aimFactory[] = {0x40,0x53,0x48,0x83,0xEC,0x20,0xB9,0xD8,0x00,0x00,0x00};
    static const uint8_t damageFactory[] = {0xE9,0x0B,0x2B,0xEB,0x13};
    static const uint8_t hostileFactory[] = {0x40,0x53,0x48,0x83,0xEC,0x20,0xB9,0x48,0x00,0x00,0x00};
    static const uint8_t graphicsFactory[] = {0x48,0x83,0xEC,0x28,0xB9,0x30,0x00,0x00,0x00};
    static const uint8_t interactionFactory[] = {0x48,0x83,0xEC,0x28,0xB9,0x30,0x00,0x00,0x00};
    static const uint8_t uiExecutionStateFactory[] = {0x48,0x83,0xEC,0x28,0xB9,0x48,0x00,0x00,0x00};
    static const uint8_t menuGraphStackOnDemandFactory[] = {0x48,0x83,0xEC,0x28,0xB9,0x70,0x00,0x00,0x00};
    static const uint8_t uiSetExecutionStateFactory[] = {0x40,0x57,0x48,0x83,0xEC,0x30,0xB9,0x40,0x00,0x00,0x00};
    static const uint8_t aimEval[] = {0x48,0x8B,0xC4,0x57,0x48,0x81,0xEC,0x90,0x00,0x00,0x00};
    static const uint8_t hostileEval[] = {0x40,0x53,0x56,0x41,0x54,0x41,0x55,0x48,0x83,0xEC,0x48};
    static const uint8_t graphicsEval[] = {0x40,0x55,0x56,0x57,0x41,0x56,0x48,0x8D,0xAC,0x24,0x68,0xFF,0xFF,0xFF};
    static const uint8_t interactionListenerEval[] = {0x48,0x89,0x5C,0x24,0x18,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
    static const uint8_t interactionModifierEval[] = {0x40,0x55,0x53,0x56,0x41,0x56,0x41,0x57,0x48,0x8D,0xAC,0x24,0xF0,0xFE,0xFF,0xFF};
    static const uint8_t uiExecutionStateEval[] = {0x4C,0x8B,0xDC,0x55,0x53,0x57,0x41,0x56,0x49,0x8D,0x6B,0xA1};
    static const uint8_t menuGraphStackOnDemandEval[] = {0x48,0x89,0x5C,0x24,0x18,0x48,0x89,0x7C,0x24,0x20,0x55,0x48,0x8D,0x6C,0x24,0xA9};
    static const uint8_t uiSetExecutionStateEval[] = {0x40,0x53,0x48,0x83,0xEC,0x40,0x8B,0x05,0x58,0x7E,0x34,0x01};
    static const uint8_t uiExecutionStateManagerSet[] = {0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xEC,0x20};
    static const uint8_t interactionListenerSetup[] = {0x48,0x89,0x6C,0x24,0x18,0x56,0x48,0x83,0xEC,0x20};
    static const uint8_t interactionCallbackDestroy[] = {0x48,0x89,0x5C,0x24,0x08,0x57,0x48,0x83,0xEC,0x20};
    static const uint8_t sequenceLevelEntityToArchive[] = {0x48,0x89,0x5C,0x24,0x18,0x57,0x48,0x83,0xEC,0x40,0x48,0x8B,0x05,0x57,0xDA,0x75};
    static const uint8_t sequenceLevelEntityFromArchive[] = {0x48,0x89,0x5C,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,0x48,0x83,0xEC,0x50};
    static const uint8_t sequenceBlockCollectionLoad[] = {0x48,0x8B,0xC4,0x48,0x89,0x50,0x10,0x55,0x48,0x8D,0xA8,0xA8,0xFC,0xFF,0xFF};
    static const uint8_t targetTrackFromArchive[] = {0x48,0x89,0x5C,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x48,0x89,0x7C,0x24,0x18,0x55,0x48,0x8D,0xAC,0x24,0x20,0xFF,0xFF,0xFF};
    static const uint8_t nixBowlGraphDispatch[] = {0x48,0x83,0xEC,0x28,0x41,0x0F,0xB6,0xC0,0x48,0x8B,0x4C,0xC2,0x20};
    static const uint8_t nixBowlInnerEvaluator[] = {0x40,0x55,0x56,0x41,0x54,0x41,0x55,0x41,0x56,0x48,0x83,0xEC,0x60};
    static const uint8_t storageGetter[] = {0x48,0x89,0x5C,0x24,0x18,0x55,0x56,0x57,0x48,0x83,0xEC,0x20};

    if (g_cfg.hookFactories) {
        CreateHook("factory PlayerAimTarget", kRvaFactoryAimTarget,
            reinterpret_cast<void*>(&HookAimFactory), reinterpret_cast<void**>(&g_origAimFactory), aimFactory, sizeof(aimFactory));
        CreateHook("factory DamageDealt", kRvaFactoryDamageDealt,
            reinterpret_cast<void*>(&HookDamageFactory), reinterpret_cast<void**>(&g_origDamageFactory), damageFactory, sizeof(damageFactory));
        CreateHook("factory IsHostile", kRvaFactoryIsHostile,
            reinterpret_cast<void*>(&HookHostileFactory), reinterpret_cast<void**>(&g_origHostileFactory), hostileFactory, sizeof(hostileFactory));
        CreateHook("factory GraphicsLines", kRvaFactoryGraphicsLines,
            reinterpret_cast<void*>(&HookLinesFactory), reinterpret_cast<void**>(&g_origLinesFactory), graphicsFactory, sizeof(graphicsFactory));
        CreateHook("factory GraphicsMultiSector", kRvaFactoryGraphicsMultiSector,
            reinterpret_cast<void*>(&HookMultiFactory), reinterpret_cast<void**>(&g_origMultiFactory), graphicsFactory, sizeof(graphicsFactory));
        if (g_cfg.hookInteractionLifecycle) {
            CreateHook("factory InteractionListener", kRvaFactoryInteractionListener,
                reinterpret_cast<void*>(&HookInteractionListenerFactory),
                reinterpret_cast<void**>(&g_origInteractionListenerFactory),
                interactionFactory, sizeof(interactionFactory));
            CreateHook("factory InteractionModifier", kRvaFactoryInteractionModifier,
                reinterpret_cast<void*>(&HookInteractionModifierFactory),
                reinterpret_cast<void**>(&g_origInteractionModifierFactory),
                interactionFactory, sizeof(interactionFactory));
        }
        if (g_cfg.hookUIExecutionState) {
            CreateHook("factory UIExecutionState", kRvaFactoryUIExecutionState,
                reinterpret_cast<void*>(&HookUIExecutionStateFactory),
                reinterpret_cast<void**>(&g_origUIExecutionStateFactory),
                uiExecutionStateFactory, sizeof(uiExecutionStateFactory));
        }
        if (g_cfg.hookMenuGraphStackOnDemand) {
            CreateHook("factory MenuGraphStack_OnDemand", kRvaFactoryMenuGraphStackOnDemand,
                reinterpret_cast<void*>(&HookMenuGraphStackOnDemandFactory),
                reinterpret_cast<void**>(&g_origMenuGraphStackOnDemandFactory),
                menuGraphStackOnDemandFactory, sizeof(menuGraphStackOnDemandFactory));
        }
        if (g_cfg.hookUISetExecutionState) {
            CreateHook("factory UI:SetExecutionState", kRvaFactoryUISetExecutionState,
                reinterpret_cast<void*>(&HookUISetExecutionStateFactory),
                reinterpret_cast<void**>(&g_origUISetExecutionStateFactory),
                uiSetExecutionStateFactory, sizeof(uiSetExecutionStateFactory));
        }
    }
    if (g_cfg.hookAimTarget) {
        CreateHook("eval PlayerAimTarget", kRvaEvalAimTarget,
            reinterpret_cast<void*>(&HookAimEval), reinterpret_cast<void**>(&g_origAimEval), aimEval, sizeof(aimEval));
    }
    if (g_cfg.hookIsHostile) {
        CreateHook("eval IsHostile", kRvaEvalIsHostile,
            reinterpret_cast<void*>(&HookHostileEval), reinterpret_cast<void**>(&g_origHostileEval), hostileEval, sizeof(hostileEval));
    }
    if (g_cfg.hookGraphics) {
        CreateHook("eval Graphics base (filtered)", kRvaEvalGraphicsBase,
            reinterpret_cast<void*>(&HookGraphicsEval), reinterpret_cast<void**>(&g_origGraphicsEval), graphicsEval, sizeof(graphicsEval));
    }
    if (g_cfg.hookInteractionLifecycle) {
        CreateHook("eval InteractionListener", kRvaEvalInteractionListener,
            reinterpret_cast<void*>(&HookInteractionListenerEval),
            reinterpret_cast<void**>(&g_origInteractionListenerEval),
            interactionListenerEval, sizeof(interactionListenerEval));
        CreateHook("eval InteractionModifier", kRvaEvalInteractionModifier,
            reinterpret_cast<void*>(&HookInteractionModifierEval),
            reinterpret_cast<void**>(&g_origInteractionModifierEval),
            interactionModifierEval, sizeof(interactionModifierEval));
    }
    if (g_cfg.hookUIExecutionState) {
        CreateHook("eval UIExecutionState", kRvaEvalUIExecutionState,
            reinterpret_cast<void*>(&HookUIExecutionStateEval),
            reinterpret_cast<void**>(&g_origUIExecutionStateEval),
            uiExecutionStateEval, sizeof(uiExecutionStateEval));
        PatchUIExecutionStateBoolCallback();
    }
    if (g_cfg.hookMenuGraphStackOnDemand) {
        CreateHook("eval MenuGraphStack_OnDemand (contracts filtered)",
            kRvaEvalMenuGraphStackOnDemand,
            reinterpret_cast<void*>(&HookMenuGraphStackOnDemandEval),
            reinterpret_cast<void**>(&g_origMenuGraphStackOnDemandEval),
            menuGraphStackOnDemandEval, sizeof(menuGraphStackOnDemandEval));
    }
    if (g_cfg.hookUISetExecutionState) {
        CreateHook("eval UI:SetExecutionState (contracts gated)",
            kRvaEvalUISetExecutionState,
            reinterpret_cast<void*>(&HookUISetExecutionStateEval),
            reinterpret_cast<void**>(&g_origUISetExecutionStateEval),
            uiSetExecutionStateEval, sizeof(uiSetExecutionStateEval));
    }
    if (g_cfg.hookUIExecutionStateManager) {
        CreateHook("UI execution-state manager set (contracts filtered)",
            kRvaUIExecutionStateManagerSet,
            reinterpret_cast<void*>(&HookUIExecutionStateManagerSet),
            reinterpret_cast<void**>(&g_origUIExecutionStateManagerSet),
            uiExecutionStateManagerSet, sizeof(uiExecutionStateManagerSet));
    }
    if (g_cfg.hookInteractionCallbacks) {
        CreateHook("setup InteractionListener callback", kRvaSetupInteractionListener,
            reinterpret_cast<void*>(&HookInteractionListenerSetup),
            reinterpret_cast<void**>(&g_origInteractionListenerSetup),
            interactionListenerSetup, sizeof(interactionListenerSetup));
        CreateHook("destroy InteractionListener callback", kRvaDestroyInteractionCallback,
            reinterpret_cast<void*>(&HookInteractionCallbackDestroy),
            reinterpret_cast<void**>(&g_origInteractionCallbackDestroy),
            interactionCallbackDestroy, sizeof(interactionCallbackDestroy));
        PatchInteractionCallbackVtable();
    }
    if (g_cfg.hookSequenceLevelEntity) {
        CreateHook("SequenceLevelEntity to archive", kRvaSequenceLevelEntityToArchive,
            reinterpret_cast<void*>(&HookSequenceLevelEntityToArchive),
            reinterpret_cast<void**>(&g_origSequenceLevelEntityToArchive),
            sequenceLevelEntityToArchive, sizeof(sequenceLevelEntityToArchive));
        CreateHook("SequenceLevelEntity from archive", kRvaSequenceLevelEntityFromArchive,
            reinterpret_cast<void*>(&HookSequenceLevelEntityFromArchive),
            reinterpret_cast<void**>(&g_origSequenceLevelEntityFromArchive),
            sequenceLevelEntityFromArchive, sizeof(sequenceLevelEntityFromArchive));
        if (g_cfg.hookSequenceBlockCollectionLoad) {
            CreateHook("Sequence block collection load (couch filtered)",
                kRvaSequenceBlockCollectionLoad,
                reinterpret_cast<void*>(&HookSequenceBlockCollectionLoad),
                reinterpret_cast<void**>(&g_origSequenceBlockCollectionLoad),
                sequenceBlockCollectionLoad, sizeof(sequenceBlockCollectionLoad));
        }
        if (g_cfg.hookTargetTrackFromArchive) {
            CreateHook("Target Track from archive (couch filtered)",
                kRvaTargetTrackFromArchive,
                reinterpret_cast<void*>(&HookTargetTrackFromArchive),
                reinterpret_cast<void**>(&g_origTargetTrackFromArchive),
                targetTrackFromArchive, sizeof(targetTrackFromArchive));
        }
    }
    if (g_cfg.hookNixBowlGraphDispatch) {
        CreateHook("Nix bowl graph dispatch (callsite filtered)",
            kRvaNixBowlGraphDispatch,
            reinterpret_cast<void*>(&HookNixBowlGraphDispatch),
            reinterpret_cast<void**>(&g_origNixBowlGraphDispatch),
            nixBowlGraphDispatch, sizeof(nixBowlGraphDispatch));
    }
    if (g_cfg.hookNixBowlInnerEvaluator) {
        CreateHook("Nix bowl inner evaluator (thread gated)",
            kRvaNixBowlInnerEvaluator,
            reinterpret_cast<void*>(&HookNixBowlInnerEvaluator),
            reinterpret_cast<void**>(&g_origNixBowlInnerEvaluator),
            nixBowlInnerEvaluator, sizeof(nixBowlInnerEvaluator));
    }
    if (g_cfg.consumerTrace) {
        CreateConsumerHook("storage bool (consumer trace)", kRvaGetBoolStorage,
            reinterpret_cast<void*>(&HookGetBoolStorage), reinterpret_cast<void**>(&g_origGetBoolStorage), storageGetter, sizeof(storageGetter));
        CreateConsumerHook("storage float (consumer trace)", kRvaGetFloatStorage,
            reinterpret_cast<void*>(&HookGetFloatStorage), reinterpret_cast<void**>(&g_origGetFloatStorage), storageGetter, sizeof(storageGetter));
        CreateConsumerHook("storage damage u32 A", kRvaGetDamageU32AStorage,
            reinterpret_cast<void*>(&HookGetDamageU32AStorage), reinterpret_cast<void**>(&g_origGetDamageU32AStorage), storageGetter, sizeof(storageGetter));
        CreateConsumerHook("storage damage u32 B", kRvaGetDamageU32BStorage,
            reinterpret_cast<void*>(&HookGetDamageU32BStorage), reinterpret_cast<void**>(&g_origGetDamageU32BStorage), storageGetter, sizeof(storageGetter));
        CreateConsumerHook("storage damage ref A", kRvaGetDamageRefAStorage,
            reinterpret_cast<void*>(&HookGetDamageRefAStorage), reinterpret_cast<void**>(&g_origGetDamageRefAStorage), storageGetter, sizeof(storageGetter));
        CreateConsumerHook("storage damage ref B", kRvaGetDamageRefBStorage,
            reinterpret_cast<void*>(&HookGetDamageRefBStorage), reinterpret_cast<void**>(&g_origGetDamageRefBStorage), storageGetter, sizeof(storageGetter));
        CreateConsumerHook("storage damage ref C", kRvaGetDamageRefCStorage,
            reinterpret_cast<void*>(&HookGetDamageRefCStorage), reinterpret_cast<void**>(&g_origGetDamageRefCStorage), storageGetter, sizeof(storageGetter));
        CreateConsumerHook("storage damage u32 C", kRvaGetDamageU32CStorage,
            reinterpret_cast<void*>(&HookGetDamageU32CStorage), reinterpret_cast<void**>(&g_origGetDamageU32CStorage), storageGetter, sizeof(storageGetter));
    }
}

void InstallPendingDamageHook() {
    if (g_damageEvalInstalled.load(std::memory_order_acquire)) return;
    void* target = g_pendingDamageEval.exchange(nullptr, std::memory_order_acq_rel);
    if (!target) return;
    const uintptr_t targetRva = ToRva(target);
    if (!targetRva) {
        Log("SKIP dynamic DamageDealt eval hook: target %p is outside Outlaws.exe", target);
        return;
    }
    const MH_STATUS create = MH_CreateHook(target, reinterpret_cast<void*>(&HookDamageEval),
        reinterpret_cast<void**>(&g_origDamageEval));
    if (create != MH_OK) {
        Log("FAIL dynamic DamageDealt hook rva=0x%llX status=%s",
            static_cast<unsigned long long>(targetRva), MH_StatusToString(create));
        return;
    }
    const MH_STATUS enable = MH_EnableHook(target);
    if (enable != MH_OK) {
        Log("FAIL enable dynamic DamageDealt hook status=%s", MH_StatusToString(enable));
        MH_RemoveHook(target);
        return;
    }
    g_damageEvalInstalled.store(true, std::memory_order_release);
    Log("hooked dynamic DamageDealt eval at rva=0x%llX (vtable slot +0x58)",
        static_cast<unsigned long long>(targetRva));
}

void WriteEventHeader() {
    if (!g_events && !g_states && !g_consumers && !g_snapshots) return;
    EnterCriticalSection(&g_logLock);
    SYSTEMTIME now = {};
    GetLocalTime(&now);
    if (g_events) {
        fprintf(g_events, "# session %04u-%02u-%02u %02u:%02u:%02u pid=%lu base=%p\n",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
            GetCurrentProcessId(), reinterpret_cast<void*>(g_base));
        fprintf(g_events, "sequence\tqpc_us\tthread\tevent\tself\tcontext\tcaller_rva\n");
        fflush(g_events);
    }
    if (g_states) {
        fprintf(g_states, "# session %04u-%02u-%02u %02u:%02u:%02u pid=%lu base=%p\n",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
            GetCurrentProcessId(), reinterpret_cast<void*>(g_base));
        fprintf(g_states, "sequence\tqpc_us\tthread\tstate\tself\tcontext\tdetails\n");
        fflush(g_states);
    }
    if (g_consumers) {
        fprintf(g_consumers, "# session %04u-%02u-%02u %02u:%02u:%02u pid=%lu base=%p\n",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
            GetCurrentProcessId(), reinterpret_cast<void*>(g_base));
        fprintf(g_consumers,
            "sequence\tqpc_us\tthread\tpulse\tslot_offset\tstorage_kind\tvalue_bits\tslot\tself\tcontext\ttag\tcaller_rva"
            "\tstack0_rva\tstack1_rva\tstack2_rva\tstack3_rva\tstack4_rva\tstack5_rva\n");
        fflush(g_consumers);
    }
    if (g_snapshots) {
        fprintf(g_snapshots, "# session %04u-%02u-%02u %02u:%02u:%02u pid=%lu base=%p test_label=%s\n",
            now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond,
            GetCurrentProcessId(), reinterpret_cast<void*>(g_base), g_cfg.testLabel);
        fprintf(g_snapshots,
            "sequence\tqpc_us\tlocal_time\treason\ttest_label\tcapture\tgame_foreground\twindow_visible"
            "\twindow_width\twindow_height\tevent_sequence\tstate_sequence\tdropped_events"
            "\tconsumer_sequence\tdropped_consumers\tgate_has_damage\tgate_fatal"
            "\taim_instances\tdamage_instances\thostile_instances\tlines_instances\tmultisector_instances"
            "\tinteraction_listener_instances\tinteraction_modifier_instances"
            "\tui_execution_state_instances\tinteraction_callback_instances"
            "\taim_states\tdamage_states\thostile_states\tlines_states\tmultisector_states"
            "\tinteraction_listener_states\tinteraction_modifier_states\tui_execution_state_states"
            "\tinteraction_callback_signals\tinteraction_callback_destroys\tcoverage\n");
        fflush(g_snapshots);
    }
    LeaveCriticalSection(&g_logLock);
}

size_t InstanceCount(InstanceSet& set) {
    AcquireSRWLockShared(&set.lock);
    const size_t count = set.count;
    ReleaseSRWLockShared(&set.lock);
    return count;
}

template <typename Snapshot>
size_t StateCount(StateCache<Snapshot>& cache) {
    AcquireSRWLockShared(&cache.lock);
    const size_t count = cache.count;
    ReleaseSRWLockShared(&cache.lock);
    return count;
}

uint64_t WriteStructuredSnapshot(const char* reason) {
    if (!g_snapshots) return 0;
    char safeReason[128] = {};
    strncpy_s(safeReason, reason ? reason : "unspecified", _TRUNCATE);
    for (char* cursor = safeReason; *cursor; ++cursor) {
        if (*cursor == '\t' || *cursor == '\r' || *cursor == '\n') *cursor = ' ';
    }

    SYSTEMTIME now = {};
    GetLocalTime(&now);
    HWND gameWindow = FindGameWindow();
    RECT bounds = {};
    const bool hasBounds = gameWindow && GetWindowRect(gameWindow, &bounds);
    DWORD foregroundProcess = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
    const bool gameForeground = foregroundProcess == GetCurrentProcessId();
    const bool windowVisible = gameWindow && IsWindowVisible(gameWindow);
    const LONG width = hasBounds ? bounds.right - bounds.left : 0;
    const LONG height = hasBounds ? bounds.bottom - bounds.top : 0;

    const size_t aimInstances = InstanceCount(g_aimInstances);
    const size_t damageInstances = InstanceCount(g_damageInstances);
    const size_t hostileInstances = InstanceCount(g_hostileInstances);
    const size_t linesInstances = InstanceCount(g_linesInstances);
    const size_t multiInstances = InstanceCount(g_multiInstances);
    const size_t interactionListenerInstances = InstanceCount(g_interactionListenerInstances);
    const size_t interactionModifierInstances = InstanceCount(g_interactionModifierInstances);
    const size_t uiExecutionStateInstances = InstanceCount(g_uiExecutionStateInstances);
    const size_t interactionCallbackInstances = InstanceCount(g_interactionCallbackInstances);
    const size_t aimStates = StateCount(g_aimStates);
    const size_t damageStates = StateCount(g_damageStates);
    const size_t hostileStates = StateCount(g_hostileStates);
    const size_t lineStates = StateCount(g_lineStates);
    const size_t multiStates = StateCount(g_multiStates);
    const size_t interactionListenerStates = StateCount(g_interactionListenerStates);
    const size_t interactionModifierStates = StateCount(g_interactionModifierStates);
    const size_t uiExecutionStateStates = StateCount(g_uiExecutionStateStates);
    const uint64_t sequence = g_snapshotSequence.fetch_add(1, std::memory_order_relaxed) + 1;

    EnterCriticalSection(&g_logLock);
    fprintf(g_snapshots,
        "%llu\t%llu\t%04u-%02u-%02uT%02u:%02u:%02u.%03u\t%s\t%s\t%u\t%u\t%u\t%ld\t%ld"
        "\t%llu\t%llu\t%llu\t%llu\t%llu\t%u\t%u"
        "\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu"
        "\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu"
        "\t%llu\t%llu\tcombat_ui_interaction_callbacks\n",
        static_cast<unsigned long long>(sequence),
        static_cast<unsigned long long>(QpcMicroseconds()),
        now.wYear, now.wMonth, now.wDay, now.wHour, now.wMinute, now.wSecond, now.wMilliseconds,
        safeReason, g_cfg.testLabel,
        g_capture.load(std::memory_order_relaxed) ? 1u : 0u,
        gameForeground ? 1u : 0u, windowVisible ? 1u : 0u, width, height,
        static_cast<unsigned long long>(g_sequence.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(g_stateSequence.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(g_dropped.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(g_consumerSequence.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(g_consumerDropped.load(std::memory_order_relaxed)),
        g_gateHasDealtDamage.load(std::memory_order_relaxed) ? 1u : 0u,
        g_gateFatal.load(std::memory_order_relaxed) ? 1u : 0u,
        static_cast<unsigned long long>(aimInstances),
        static_cast<unsigned long long>(damageInstances),
        static_cast<unsigned long long>(hostileInstances),
        static_cast<unsigned long long>(linesInstances),
        static_cast<unsigned long long>(multiInstances),
        static_cast<unsigned long long>(interactionListenerInstances),
        static_cast<unsigned long long>(interactionModifierInstances),
        static_cast<unsigned long long>(uiExecutionStateInstances),
        static_cast<unsigned long long>(interactionCallbackInstances),
        static_cast<unsigned long long>(aimStates),
        static_cast<unsigned long long>(damageStates),
        static_cast<unsigned long long>(hostileStates),
        static_cast<unsigned long long>(lineStates),
        static_cast<unsigned long long>(multiStates),
        static_cast<unsigned long long>(interactionListenerStates),
        static_cast<unsigned long long>(interactionModifierStates),
        static_cast<unsigned long long>(uiExecutionStateStates),
        static_cast<unsigned long long>(g_interactionCallbackSignals.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(g_interactionCallbackDestroys.load(std::memory_order_relaxed)));
    fflush(g_snapshots);
    LeaveCriticalSection(&g_logLock);
    Log("structured snapshot %llu written (%s), coverage=combat_ui_interaction_callbacks",
        static_cast<unsigned long long>(sequence), safeReason);
    return sequence;
}

void SetCapture(bool enabled, const char* reason) {
    if (enabled) {
        ResetStateCaches();
        ResetConsumerTraceCaches();
        SetConsumerHooksEnabled(true);
    } else {
        DumpContractsStateManager("capture_stop");
        DumpUISetExecutionWindow("capture_stop");
    }
    g_capture.store(enabled, std::memory_order_relaxed);
    if (!enabled) {
        g_contractsSetExecutionWindow.store(false, std::memory_order_relaxed);
        SetConsumerHooksEnabled(false);
    }
    if (!enabled && (g_events || g_states || g_consumers || g_snapshots)) {
        EnterCriticalSection(&g_logLock);
        if (g_events) fflush(g_events);
        if (g_states) fflush(g_states);
        if (g_consumers) fflush(g_consumers);
        if (g_snapshots) fflush(g_snapshots);
        LeaveCriticalSection(&g_logLock);
    }
    Log("capture %s (%s), events=%llu dropped=%llu", enabled ? "ON" : "OFF", reason,
        static_cast<unsigned long long>(g_sequence.load()),
        static_cast<unsigned long long>(g_dropped.load()));
    if (g_cfg.automaticSnapshots && _stricmp(reason, "startup setting") != 0)
        WriteStructuredSnapshot(enabled ? "capture_start" : "capture_stop");
    if (_stricmp(reason, "startup setting") != 0)
        ShowOverlay(enabled ? "OBSERVER: CAPTURE ON" : "OBSERVER: CAPTURE OFF");
}

void ManualMarker() {
    if (!g_events && !g_states && !g_consumers) return;
    const uint32_t marker = g_markerNumber.fetch_add(1) + 1;
    char eventName[64] = {};
    char overlayText[128] = {};
    snprintf(eventName, sizeof(eventName), "MANUAL_MARKER_%u", marker);
    snprintf(overlayText, sizeof(overlayText), "OBSERVER: MARKER %u", marker);
    const bool wasCapturing = g_capture.exchange(true);
    Trace(eventName, nullptr, nullptr, _ReturnAddress());
    StateTrace(eventName, nullptr, nullptr, "marker=%u", marker);
    DumpContractsStateManager(eventName);
    DumpUISetExecutionWindow(eventName);
    if (!wasCapturing) g_capture.store(false);
    if (g_cfg.automaticSnapshots) WriteStructuredSnapshot(eventName);
    EnterCriticalSection(&g_logLock);
    if (g_events) fflush(g_events);
    if (g_states) fflush(g_states);
    if (g_consumers) fflush(g_consumers);
    if (g_snapshots) fflush(g_snapshots);
    LeaveCriticalSection(&g_logLock);
    Log("manual marker %u written at qpc_us=%llu", marker,
        static_cast<unsigned long long>(QpcMicroseconds()));
    ShowOverlay(overlayText);
    if (g_cfg.audioMarker) Beep(1200, 90);
}

void ToggleDiagnosticGate(std::atomic<bool>& gate, const char* name) {
    const bool enabled = !gate.load(std::memory_order_relaxed);
    gate.store(enabled, std::memory_order_relaxed);
    char overlayText[128] = {};
    snprintf(overlayText, sizeof(overlayText), "OBSERVER: GATE %s %s", name,
        enabled ? "ON" : "OFF");
    Log("diagnostic gate %s %s (applies only during F8 capture)", name,
        enabled ? "ON" : "OFF");
    StateTrace("DIAGNOSTIC_GATE", nullptr, nullptr, "name=%s;enabled=%u", name,
        enabled ? 1u : 0u);
    ShowOverlay(overlayText);
    if (g_cfg.audioMarker) Beep(enabled ? 900 : 650, 90);
}

void DumpKnownInstances() {
    const struct { NodeKind kind; InstanceSet* set; } sets[] = {
        {NodeKind::AimTarget, &g_aimInstances},
        {NodeKind::DamageDealt, &g_damageInstances},
        {NodeKind::IsHostile, &g_hostileInstances},
        {NodeKind::GraphicsLines, &g_linesInstances},
        {NodeKind::GraphicsMultiSector, &g_multiInstances},
        {NodeKind::InteractionListener, &g_interactionListenerInstances},
        {NodeKind::InteractionModifier, &g_interactionModifierInstances},
        {NodeKind::UIExecutionState, &g_uiExecutionStateInstances},
        {NodeKind::MenuGraphStackOnDemand, &g_menuGraphStackOnDemandInstances},
        {NodeKind::UISetExecutionState, &g_uiSetExecutionStateInstances},
    };
    for (const auto& item : sets) {
        AcquireSRWLockShared(&item.set->lock);
        Log("snapshot %s instances=%llu", KindName(item.kind),
            static_cast<unsigned long long>(item.set->count));
        if (item.set->count) DumpVtable(KindName(item.kind), item.set->items[0]);
        if (item.kind == NodeKind::MenuGraphStackOnDemand) {
            const size_t limit = item.set->count < 256 ? item.set->count : 256;
            for (size_t index = 0; index < limit; ++index)
                DumpOnDemandObject(item.set->items[index]);
            if (item.set->count > limit) {
                Log("snapshot %s object dump truncated at %llu of %llu instances",
                    KindName(item.kind), static_cast<unsigned long long>(limit),
                    static_cast<unsigned long long>(item.set->count));
            }
        }
        ReleaseSRWLockShared(&item.set->lock);
    }
    AcquireSRWLockShared(&g_interactionCallbackInstances.lock);
    Log("snapshot InteractionListener callbacks active=%llu signals=%llu destroys=%llu",
        static_cast<unsigned long long>(g_interactionCallbackInstances.count),
        static_cast<unsigned long long>(g_interactionCallbackSignals.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(g_interactionCallbackDestroys.load(std::memory_order_relaxed)));
    if (g_interactionCallbackInstances.count)
        DumpVtable("InteractionListener callback", g_interactionCallbackInstances.items[0]);
    ReleaseSRWLockShared(&g_interactionCallbackInstances.lock);
    Log("snapshot interaction evaluator transitions listener=%llu modifier=%llu details=%s",
        static_cast<unsigned long long>(
            g_interactionListenerTransitions.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            g_interactionModifierTransitions.load(std::memory_order_relaxed)),
        g_cfg.interactionTransitionDetails ? "ON" : "OFF");
    Log("snapshot interaction observed states listener=%llu modifier=%llu details=%s",
        static_cast<unsigned long long>(
            g_interactionListenerObservations.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            g_interactionModifierObservations.load(std::memory_order_relaxed)),
        g_cfg.interactionObservedStateDetails ? "ON" : "OFF");
    Log("snapshot Nix bowl graph dispatch open=%llu close=%llu hook=%s",
        static_cast<unsigned long long>(
            g_nixBowlGraphOpenCalls.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            g_nixBowlGraphCloseCalls.load(std::memory_order_relaxed)),
        g_cfg.hookNixBowlGraphDispatch ? "ON" : "OFF");
    Log("snapshot Nix bowl inner evaluator open=%llu close=%llu "
        "caller_mismatches=%llu hook=%s",
        static_cast<unsigned long long>(
            g_nixBowlInnerOpenCalls.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            g_nixBowlInnerCloseCalls.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            g_nixBowlInnerCallerMismatches.load(std::memory_order_relaxed)),
        g_cfg.hookNixBowlInnerEvaluator ? "ON" : "OFF");
    Log("snapshot SequenceLevelEntity codec to_archive=%llu from_archive=%llu "
        "couch_matches=%llu couch_seen=%s collection_loads=%llu "
        "target_track_loads=%llu",
        static_cast<unsigned long long>(
            g_sequenceLevelEntityToArchiveCalls.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            g_sequenceLevelEntityFromArchiveCalls.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            g_sequenceLevelEntityCouchMatches.load(std::memory_order_relaxed)),
        g_sequenceLevelEntityCouchLogged.load(std::memory_order_relaxed) ? "YES" : "NO",
        static_cast<unsigned long long>(
            g_sequenceBlockCouchLoads.load(std::memory_order_relaxed)),
        static_cast<unsigned long long>(
            g_targetTrackCouchLoads.load(std::memory_order_relaxed)));
    Log("snapshot couch UID watch requested=%s armed=%s targets=%llu threads=%llu "
        "hits=%u drained=%u dropped=%u stale_ignored=%u candidate_sequence=%llu",
        g_uidWatchRequested.load(std::memory_order_relaxed) ? "YES" : "NO",
        g_uidWatchArmed.load(std::memory_order_relaxed) ? "YES" : "NO",
        static_cast<unsigned long long>(g_uidWatchTargetCount),
        static_cast<unsigned long long>(g_uidWatchThreadCount),
        g_uidWatchWriteIndex.load(std::memory_order_relaxed), g_uidWatchDrainIndex,
        g_uidWatchDropped.load(std::memory_order_relaxed),
        g_uidWatchStaleIgnored.load(std::memory_order_relaxed),
        static_cast<unsigned long long>(
            g_uidWatchCandidateSequence.load(std::memory_order_relaxed)));
}

void ManualSnapshot() {
    DumpKnownInstances();
    const uint64_t sequence = WriteStructuredSnapshot("manual_F10");
    char overlayText[128] = {};
    snprintf(overlayText, sizeof(overlayText), "OBSERVER: SNAPSHOT %llu",
        static_cast<unsigned long long>(sequence));
    ShowOverlay(overlayText);
    if (g_cfg.audioMarker) Beep(760, 90);
}

DWORD WINAPI WorkerThread(void*) {
    ResolvePaths(g_module);
    QueryPerformanceFrequency(&g_qpcFrequency);
    QueryPerformanceCounter(&g_qpcStart);
    OpenLogs();
    ReadConfig();
    g_gateHasDealtDamage.store(g_cfg.gateHasDealtDamage, std::memory_order_relaxed);
    g_gateFatal.store(g_cfg.gateFatal, std::memory_order_relaxed);
    Log("========== Outlaws Event Observer session start (pid %lu) ==========", GetCurrentProcessId());
    Log("dll=%s", g_dllDir);
    Log("ini=%s", g_iniPath);
    Log("events=%s", g_eventPath);
    Log("states=%s", g_statePath);
    Log("consumers=%s", g_consumerPath);
    Log("snapshots=%s", g_snapshotPath);
    Log("test label=%s automatic snapshots=%s", g_cfg.testLabel,
        g_cfg.automaticSnapshots ? "ON" : "OFF");
    Log("consumer trace=%s window=%lu ms max=%llu couch UID watch=%s",
        g_cfg.consumerTrace ? "ON" : "OFF", g_cfg.consumerTraceWindowMs,
        static_cast<unsigned long long>(g_cfg.maxConsumerEvents),
        g_cfg.couchUidWatch ? "ON" : "OFF");
    Log("interaction lifecycle=%s callbacks=%s UI execution state=%s "
        "MenuGraphStack OnDemand=%s UI:SetExecutionState=%s "
        "UI execution-state manager=%s SequenceLevelEntity=%s "
        "SequenceBlockCollectionLoad=%s TargetTrackFromArchive=%s evaluator calls=%s "
        "transition details=%s observed-state details=%s Nix bowl graph dispatch=%s "
        "Nix bowl inner evaluator=%s",
        g_cfg.hookInteractionLifecycle ? "ON" : "OFF",
        g_cfg.hookInteractionCallbacks ? "ON" : "OFF",
        g_cfg.hookUIExecutionState ? "ON" : "OFF",
        g_cfg.hookMenuGraphStackOnDemand ? "ON" : "OFF",
        g_cfg.hookUISetExecutionState ? "ON" : "OFF",
        g_cfg.hookUIExecutionStateManager ? "ON" : "OFF",
        g_cfg.hookSequenceLevelEntity ? "ON" : "OFF",
        g_cfg.hookSequenceBlockCollectionLoad ? "ON" : "OFF",
        g_cfg.hookTargetTrackFromArchive ? "ON" : "OFF",
        g_cfg.interactionEvaluatorCalls ? "ON" : "OFF",
        g_cfg.interactionTransitionDetails ? "ON" : "OFF",
        g_cfg.interactionObservedStateDetails ? "ON" : "OFF",
        g_cfg.hookNixBowlGraphDispatch ? "ON" : "OFF",
        g_cfg.hookNixBowlInnerEvaluator ? "ON" : "OFF");
    Log("hotkeys: F5 couch UID watch, F6 HasDealtDamage gate, F7 Fatal gate, "
        "F8 capture, F9 marker, F10 structured snapshot");
    Log("initial diagnostic gates: HasDealtDamage=%s Fatal=%s",
        g_cfg.gateHasDealtDamage ? "ON" : "OFF", g_cfg.gateFatal ? "ON" : "OFF");
    if (!g_cfg.enabled) {
        Log("observer disabled by INI");
        return 0;
    }
    if (g_cfg.startupDelayMs) Sleep(g_cfg.startupDelayMs);

    MODULEINFO moduleInfo = {};
    HMODULE executable = GetModuleHandleA(nullptr);
    if (!GetModuleInformation(GetCurrentProcess(), executable, &moduleInfo, sizeof(moduleInfo))) {
        Log("unable to query Outlaws.exe module information");
        return 0;
    }
    g_base = reinterpret_cast<uintptr_t>(moduleInfo.lpBaseOfDll);
    g_imageSize = moduleInfo.SizeOfImage;
    MODULEINFO observerInfo = {};
    if (GetModuleInformation(GetCurrentProcess(), g_module, &observerInfo,
            sizeof(observerInfo))) {
        g_observerBase = reinterpret_cast<uintptr_t>(observerInfo.lpBaseOfDll);
        g_observerSize = observerInfo.SizeOfImage;
    }
    WriteEventHeader();
    const bool exactBuild = ValidateBuild();
    if (!exactBuild && g_cfg.exactBuildOnly) {
        Log("refusing to install hooks because ExactBuildOnly=true");
        return 0;
    }

    Log("known static vtables: Aim=0x%llX Hostile=0x%llX Lines=0x%llX Multi=0x%llX "
        "InteractionListener=0x%llX InteractionModifier=0x%llX UIExecutionState=0x%llX "
        "InteractionCallback=0x%llX",
        static_cast<unsigned long long>(kRvaVtableAimTarget),
        static_cast<unsigned long long>(kRvaVtableIsHostile),
        static_cast<unsigned long long>(kRvaVtableGraphicsLines),
        static_cast<unsigned long long>(kRvaVtableGraphicsMultiSector),
        static_cast<unsigned long long>(kRvaVtableInteractionListener),
        static_cast<unsigned long long>(kRvaVtableInteractionModifier),
        static_cast<unsigned long long>(kRvaVtableUIExecutionState),
        static_cast<unsigned long long>(kRvaVtableInteractionCallback));
    const MH_STATUS init = MH_Initialize();
    if (init != MH_OK && init != MH_ERROR_ALREADY_INITIALIZED) {
        Log("MinHook initialization failed: %s", MH_StatusToString(init));
        return 0;
    }
    InstallKnownHooks();
    SetCapture(g_cfg.captureOnStart, "startup setting");
    CreateOverlay();
    if (g_cfg.automaticSnapshots) WriteStructuredSnapshot("observer_ready");
    ShowOverlay("OBSERVER READY - F8 STARTS CAPTURE", 2200);

    bool f5WasDown = false;
    bool f6WasDown = false;
    bool f7WasDown = false;
    bool f8WasDown = false;
    bool f9WasDown = false;
    bool f10WasDown = false;
    while (g_running.load(std::memory_order_relaxed)) {
        PumpOverlay();
        InstallPendingDamageHook();
        DrainUidWatchHits();
        if (g_uidWatchStopRequested.load(std::memory_order_acquire) &&
            g_uidWatchArmed.load(std::memory_order_acquire)) {
            RestoreUidWatchThreads();
            g_uidWatchRequested.store(false, std::memory_order_release);
            g_uidWatchStopRequested.store(false, std::memory_order_relaxed);
            ShowOverlay("COUCH UID WATCH COMPLETE", 2200);
        }
        if (g_uidWatchRequested.load(std::memory_order_acquire)) {
            if (!g_uidWatchArmed.load(std::memory_order_acquire)) {
                if (ArmUidWatch()) ShowOverlay("COUCH UID WATCH ARMED", 2200);
            } else if (g_uidWatchCandidateSequence.load(std::memory_order_acquire) >
                g_uidWatchArmedCandidateSequence.load(std::memory_order_acquire)) {
                RefreshUidWatchTargets();
            }
        }
        void* contractsManager = g_contractsUIExecutionStateManager.load(
            std::memory_order_relaxed);
        if (contractsManager) {
            SampleContractsStateManager("worker", contractsManager, 0xFFFFFFFFu,
                nullptr);
        }
        const bool f5Down = (GetAsyncKeyState(VK_F5) & 0x8000) != 0;
        const bool f6Down = (GetAsyncKeyState(VK_F6) & 0x8000) != 0;
        const bool f7Down = (GetAsyncKeyState(VK_F7) & 0x8000) != 0;
        const bool f8Down = (GetAsyncKeyState(VK_F8) & 0x8000) != 0;
        const bool f9Down = (GetAsyncKeyState(VK_F9) & 0x8000) != 0;
        const bool f10Down = (GetAsyncKeyState(VK_F10) & 0x8000) != 0;
        if (f5Down && !f5WasDown) ToggleUidWatch();
        if (f6Down && !f6WasDown) ToggleDiagnosticGate(g_gateHasDealtDamage, "HAS DAMAGE");
        if (f7Down && !f7WasDown) ToggleDiagnosticGate(g_gateFatal, "FATAL");
        if (f8Down && !f8WasDown) SetCapture(!g_capture.load(), "F8");
        if (f9Down && !f9WasDown) ManualMarker();
        if (f10Down && !f10WasDown) ManualSnapshot();
        f5WasDown = f5Down;
        f6WasDown = f6Down;
        f7WasDown = f7Down;
        f8WasDown = f8Down;
        f9WasDown = f9Down;
        f10WasDown = f10Down;
        Sleep((g_contractsSetExecutionWindow.load(std::memory_order_relaxed) ||
            g_uidWatchRequested.load(std::memory_order_relaxed)) ? 2 : 20);
    }
    return 0;
}

} // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
        HANDLE thread = CreateThread(nullptr, 0, WorkerThread, nullptr, 0, nullptr);
        if (thread) CloseHandle(thread);
    } else if (reason == DLL_PROCESS_DETACH) {
        g_running.store(false, std::memory_order_relaxed);
    }
    return TRUE;
}

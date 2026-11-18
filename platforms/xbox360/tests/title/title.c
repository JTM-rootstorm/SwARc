// The runtime test title (tests/CMakeLists.txt, xbox360_runtime): a
// program that calls the kernel as a title does, through imports, and
// checks what the kernel's documented behaviour gives. Built for the
// 64-bit ELFv2 ABI like the fixture, so pointers are 64-bit here while
// the guest's structures hold 32-bit ones (uint32_t fields), and calls
// into the kernel pass at most eight arguments: ELFv2's parameter area is
// not the Xenon's (start.S's XENON9 and XENON10 wrappers make the calls
// that need more). Reports each failed check with DbgPrint and returns
// their number, which the title's end makes the process's exit status.

#include <stdint.h>

typedef uint32_t u32;

// The imports (imports.S). Every result is declared unsigned: the kernel
// returns 32-bit values, zero-extended here.
u32 DbgPrint(const char* fmt, ...);
u32 ExAllocatePool(u32 size);
u32 ExCreateThread(u32* handle, u32 stack, u32* id, void* startup, void* start, void* context, u32 flags);
u32 ExFreePool(u32 p);
u32 InterlockedFlushSList(void* head);
u32 InterlockedPopEntrySList(void* head);
u32 KeDelayExecutionThread(u32 mode, u32 alertable, int64_t* interval);
u32 KeGetCurrentProcessType(void);
u32 KeLockL2(u32 index, u32 va, u32 size, u32 a, u32 b);
u32 KeUnlockL2(u32 index);
u32 KeInitializeSemaphore(void* sem, u32 count, u32 limit);
uint64_t KeQueryPerformanceFrequency(void);
u32 KeQuerySystemTime(uint64_t* time);
u32 KeReleaseSemaphore(void* sem, u32 increment, u32 adjustment, u32 wait);
u32 KeResetEvent(void* event);
u32 KeSetEvent(void* event, u32 increment, u32 wait);
u32 KeWaitForSingleObject(void* obj, u32 reason, u32 mode, u32 alertable, int64_t* timeout);
u32 KfAcquireSpinLock(u32* lock);
u32 KfReleaseSpinLock(u32* lock, u32 irql);
u32 MmAllocatePhysicalMemoryEx(u32 flags, u32 size, u32 protect, u32 lo, u32 hi, u32 align);
u32 MmFreePhysicalMemory(u32 type, u32 va);
u32 MmGetPhysicalAddress(u32 va);
u32 MmQueryAddressProtect(u32 va);
u32 NtAllocateVirtualMemory(u32* base, u32* size, u32 type, u32 protect, u32 debug);
u32 NtClose(u32 handle);
u32 NtCreateEvent(u32* handle, void* attributes, u32 type, u32 state);
u32 NtCreateMutant(u32* handle, void* attributes, u32 owned);
u32 NtFreeVirtualMemory(u32* base, u32* size, u32 type, u32 debug);
u32 NtQueryVirtualMemory(u32 base, void* info, u32 debug);
u32 NtReleaseMutant(u32 handle, u32* previous);
u32 NtResumeThread(u32 handle, u32* previous);
u32 NtSuspendThread(u32 handle, u32* previous);
u32 NtSetEvent(u32 handle, u32* previous);
u32 NtWaitForSingleObjectEx(u32 handle, u32 mode, u32 alertable, int64_t* timeout);
u32 NtWaitForMultipleObjectsEx(u32 count, u32* handles, u32 type, u32 mode, u32 alertable, int64_t* timeout);
u32 ObReferenceObjectByHandle(u32 handle, u32 type, u32* object);
u32 ObDereferenceObject(u32 object);
u32 RtlEnterCriticalSection(void* cs);
u32 RtlFillMemoryUlong(void* dest, u32 length, u32 pattern);
u32 RtlInitAnsiString(void* dest, const char* src);
u32 RtlInitializeCriticalSection(void* cs);
u32 RtlLeaveCriticalSection(void* cs);
u32 RtlTryEnterCriticalSection(void* cs);
u32 KeTlsAlloc(void);
u32 KeTlsGetValue(u32 index);
u32 KeTlsFree(u32 index);
u32 NtPulseEvent(u32 handle, u32* previous);
u32 RtlTimeFieldsToTime(const uint16_t* fields, uint64_t* time);
u32 KeTlsSetValue(u32 index, u32 value);
u32 sprintf(char* buf, const char* fmt, ...);
u32 NtCreateFile(void);  // through NtCreateFile9
u32 NtQueryDirectoryFile(void);  // through NtQueryDirectoryFile9
u32 NtFlushBuffersFile(u32 handle, u32* io);
u32 NtOpenFile(u32* handle, u32 access, void* attributes, u32* io, u32 share, u32 options);
u32 NtQueryFullAttributesFile(void* attributes, void* info);
u32 NtQueryInformationFile(u32 handle, u32* io, void* info, u32 length, u32 cls);
u32 NtQueryVolumeInformationFile(u32 handle, u32* io, void* info, u32 length, u32 cls);
u32 NtReadFile(u32 handle, u32 event, void* apc, void* context, u32* io, void* buf, u32 length, int64_t* offset);
u32 NtSetInformationFile(u32 handle, u32* io, void* info, u32 length, u32 cls);
u32 NtWriteFile(u32 handle, u32 event, void* apc, void* context, u32* io, const void* buf, u32 length,
                int64_t* offset);
u32 ObCreateSymbolicLink(void* link, void* target);
u32 ObDeleteSymbolicLink(void* link);
u32 _vsnprintf(char* buf, u32 count, const char* fmt, uint64_t* args);
u32 RtlImageXexHeaderField(u32 headers, u32 key);
u32 XexCheckExecutablePrivilege(u32 privilege);
u32 XexGetModuleHandle(const char* name, u32* handle);
u32 XexGetModuleSection(u32 handle, const char* name, u32* data, u32* size);
u32 XexGetProcedureAddress(u32 handle, u32 ordinal, u32* address);
u32 ExGetXConfigSetting(u32 category, u32 setting, void* buffer, u32 size, uint16_t* required);
u32 ExRegisterTitleTerminateNotification(void* registration, u32 create);
u32 FscSetCacheElementCount(u32 count);
u32 HalGetCurrentAVPack(void);
u32 VdQueryVideoMode(u32* mode);
u32 VdEnableRingBufferRPtrWriteBack(u32 pa, u32 block_log2);
u32 VdInitializeRingBuffer(u32 pa, u32 size_log2);
u32 VdSetGraphicsInterruptCallback(void* callback, u32 user);
u32 KeEnableFpuExceptions(u32 enable);
u32 RtlMultiByteToUnicodeN(uint16_t* dest, u32 dest_bytes, u32* written, const char* src, u32 src_bytes);
u32 RtlUnicodeToMultiByteN(char* dest, u32 dest_bytes, u32* written, const uint16_t* src, u32 src_bytes);
u32 RtlUnicodeStringToAnsiString(void* ansi, void* unicode, u32 allocate);
u32 RtlFreeAnsiString(void* ansi);
u32 RtlNtStatusToDosError(u32 status);
u32 RtlTimeToTimeFields(const uint64_t* time, uint16_t* fields);
// seh.S
u32 seh_outer(u32 code);
u32 own_frame(u32 code);
u32 capture(void* context);
u32 back_outer(void);
u32 sj_caller(void);
extern u32 seh_log[16];
u32 XAudioRegisterRenderDriverClient(u32* registration, u32* driver);
u32 XAudioUnregisterRenderDriverClient(u32 driver);
u32 XAudioSubmitRenderDriverFrame(u32 driver, float* samples);
u32 XAudioGetVoiceCategoryVolumeChangeMask(u32 driver, u32* mask);
u32 XAudioGetVoiceCategoryVolume(u32 category, u32* volume);
u32 XMACreateContext(u32* context);
u32 XMAReleaseContext(u32 context);
u32 MmQueryStatistics(u32* statistics);
u32 NtCreateTimer(u32* handle, void* attributes, u32 type);
u32 NtSetTimerEx(u32 handle, int64_t* due, void* apc, u32 mode, void* context, u32 resume, u32 period, u32* previous);
u32 NtCancelTimer(u32 handle, u32* current);
u32 KeInitializeApc(void* apc, u32 thread, void* kernel, void* rundown, void* normal, u32 mode, u32 context);
u32 KeInsertQueueApc(void* apc, u32 arg1, u32 arg2, u32 increment);
u32 NtQueueApcThread(u32 thread, void* routine, u32 context, u32 arg1, u32 arg2);
u32 KeEnterCriticalRegion(void);
u32 KeLeaveCriticalRegion(void);
u32 XGetAVPack(void);     // xam
u32 XGetVideoMode(u32* mode);  // xam
u32 XGetGameRegion(void);
u32 XamGetSystemVersion(void);
u32 XamLoaderGetLaunchDataSize(u32* size);
u32 XamLoaderGetLaunchData(void* buffer, u32 size);
u32 NetDll_WSAStartup(u32 caller, u32 version, void* data);
u32 NetDll_XNetStartup(u32 caller, void* parameters);
u32 NetDll_XNetGetEthernetLinkStatus(u32 caller);
u32 XamInputGetCapabilities(u32 user, u32 flags, void* caps);
u32 XamInputGetState(u32 user, u32 flags, void* state);
u32 XamInputSetState(u32 user, u32 flags, void* vibration);
u32 XamAlloc(u32 attributes, u32 size, u32* address);
u32 XamFree(u32 address);
u32 XMsgInProcessCall(u32 app, u32 message, void* p1, void* p2);
u32 XamUserGetSigninState(u32 slot);
u32 XamUserGetXUID(u32 slot, u32 types, uint64_t* xuid);
u32 XamUserGetName(u32 slot, char* buffer, u32 size);
u32 XamUserReadProfileSettings(void);  // through XamUserReadProfileSettings9
u32 XamUserWriteProfileSettings(u32 title, u32 slot, u32 count, void* settings, u32* overlapped);
u32 XamNotifyCreateListener(uint64_t areas, u32 version);
u32 XNotifyGetNext(u32 listener, u32 match, u32* id, u32* value);
u32 XamUserCheckPrivilege(u32 slot, u32 privilege, u32* result);
u32 XamShowSigninUI(u32 panes, u32 flags);
u32 XamSessionCreateHandle(u32* handle);
u32 XamSessionRefObjByHandle(u32 handle, u32* object);
u32 XNotifyPositionUI(u32 position);
u32 XMsgStartIORequest(u32 app, u32 message, u32* overlapped, void* buffer, u32 size);
u32 XMsgStartIORequestEx(u32 app, u32 message, u32* overlapped, void* buffer, u32 size, void* xparams);
u32 XamEnumerate(u32 handle, u32 flags, void* buffer, u32 size, u32* returned, u32* overlapped);
u32 XamContentCreateEx(void);  // through XamContentCreateEx9
u32 XamContentClose(const char* root, u32* overlapped);
u32 XamContentDelete(u32 user, void* data, u32* overlapped);
u32 XamContentCreateEnumerator(u32 user, u32 device, u32 type, u32 flags, u32 items, u32* size, u32* handle);
u32 XamContentGetDeviceData(u32 device, void* data);
u32 XamContentGetCreator(u32 user, void* data, u32* is_creator, uint64_t* creator, u32* overlapped);
u32 XamContentGetDeviceState(u32 device, u32* overlapped);
u32 XGetLanguage(void);
u32 XamGetExecutionId(u32* info);
u32 XNetLogonGetTitleID(u32 service);
u32 XMsgCancelIORequest(u32* overlapped, u32 wait);
u32 XamUserAreUsersFriends(u32 slot, uint64_t* xuids, u32 count, u32* result, u32* overlapped);
u32 XamWriteGamerTile(u32 slot, u32 title, u32 large, u32 small, u32 award, u32* overlapped);
u32 XamUserCreateStatsEnumerator(u32 title, u32 kind, uint64_t pivot, u32 rows, u32 count, void* views, u32* size,
                                 u32* handle);
u32 XamShowFriendsUI(u32 slot);
u32 XamShowPlayerReviewUI(u32 slot, uint64_t xuid);
u32 XamShowAchievementsUI(u32 slot, u32 title);
u32 XamShowGamerCardUIForXUID(u32 slot, uint64_t xuid, u32 flags);
u32 XamShowMarketplaceUI(u32 slot, u32 type, uint64_t offer, u32 categories);
u32 XamShowKeyboardUI(u32 slot, u32 flags, const uint16_t* text, const uint16_t* title, const uint16_t* description,
                      uint16_t* buffer, u32 chars, u32* overlapped);
u32 XamShowMessageBoxUIEx(void);  // through XamShowMessageBoxUIEx9
u32 XamShowDirtyDiscErrorUI(void);
u32 XamVoiceCreate(u32 slot, u32 packets, u32* voice);
u32 XamVoiceHeadsetPresent(u32 voice);
u32 XamVoiceSubmitPacket(u32 voice, u32 size, void* packet);
u32 XamVoiceClose(u32 voice);
u32 NetDll_WSACleanup(u32 caller);
u32 NetDll_socket(u32 caller, u32 family, u32 type, u32 protocol);
u32 NetDll_closesocket(u32 caller, u32 s);
u32 NetDll_ioctlsocket(u32 caller, u32 s, u32 command, u32* arg);
u32 NetDll_setsockopt(u32 caller, u32 s, u32 level, u32 option, const void* value, u32 length);
u32 NetDll_getsockopt(u32 caller, u32 s, u32 level, u32 option, void* value, u32* length);
u32 NetDll_getsockname(u32 caller, u32 s, void* name, u32* length);
u32 NetDll_bind(u32 caller, u32 s, const void* name, u32 length);
u32 NetDll_connect(u32 caller, u32 s, const void* name, u32 length);
u32 NetDll_recv(u32 caller, u32 s, void* buffer, u32 length, u32 flags);
u32 NetDll_recvfrom(u32 caller, u32 s, void* buffer, u32 length, u32 flags, void* from, u32* from_length);
u32 NetDll_send(u32 caller, u32 s, const void* buffer, u32 length, u32 flags);
u32 NetDll_sendto(u32 caller, u32 s, const void* buffer, u32 length, u32 flags, const void* to, u32 to_length);
u32 NetDll_inet_addr(const char* text);
u32 NetDll_WSAGetLastError(void);
u32 NetDll_WSACreateEvent(void);
u32 NetDll_XNetCleanup(u32 caller);
u32 NetDll_XNetRandom(u32 caller, void* buffer, u32 length);
u32 NetDll_XNetCreateKey(u32 caller, void* id, void* key);
u32 NetDll_XNetRegisterKey(u32 caller, const void* id, const void* key);
u32 NetDll_XNetUnregisterKey(u32 caller, const void* id);
u32 NetDll_XNetXnAddrToInAddr(u32 caller, const void* xnaddr, const void* id, u32* ina);
u32 NetDll_XNetDnsLookup(u32 caller, const char* name, u32 event, u32* xndns);
u32 NetDll_XNetDnsRelease(u32 caller, u32 xndns);
u32 NetDll_XNetQosListen(u32 caller, const void* id, const void* data, u32 size, u32 bits, u32 flags);
u32 NetDll_XNetQosLookup(void);  // through NetDll_XNetQosLookup13
u32 NetDll_XNetQosRelease(u32 caller, u32 xnqos);
u32 NetDll_XNetGetTitleXnAddr(u32 caller, void* xnaddr);
u32 NetDll_XNetGetDebugXnAddr(u32 caller, void* xnaddr);
u32 XamShowDeviceSelectorUI(u32 user, u32 type, u32 flags, uint64_t bytes, u32* device, u32* overlapped);
extern u32 __imp_NtClose, __imp_ExThreadObjectType, __imp_KeTimeStampBundle, __imp_XboxKrnlVersion,
    __imp_XexExecutableModuleHandle, __imp_ExLoadedCommandLine, __imp_VdGlobalDevice, __imp_VdGpuClockInMHz,
    __imp_VdHSIOCalibrationLock;

// start.S
void _start(void);
void thread_startup(void);
u32 sprintf9(char* buf, const char* fmt, int a, int b, int c, int d, int e, int f, int g);
u32 NtCreateFile9(u32* handle, u32 access, void* attributes, u32* io, void* size, u32 file_attributes,
                  u32 share, u32 disposition, u32 options);
u32 NtQueryDirectoryFile9(u32 handle, u32 event, void* apc, void* context, u32* io, void* info, u32 length,
                          void* mask, u32 restart);
u32 XamUserReadProfileSettings9(u32 title, u32 slot, u32 xuid_count, void* xuids, u32 count, u32* ids, u32* size,
                                void* results, u32* overlapped);
u32 XamShowMessageBoxUIEx9(u32 slot, const uint16_t* title, const uint16_t* text, u32 count, const uint16_t** buttons,
                           u32 focus, u32 flags, u32* result, u32* overlapped);
u32 XamContentCreateEx9(u32 user, const char* root, void* data, u32 flags, u32* disposition, u32* license,
                        u32 cache_size, uint64_t content_size, u32* overlapped);
u32 NetDll_XNetQosLookup13(u32 caller, u32 cxna, u32* xnaddrs, u32* ids, u32* keys, u32 cina, u32* inas,
                           u32* services, u32 probes, u32 bits, u32 flags, u32 event, u32* xnqos);
u32 VdSwap10(volatile u32* words, u32* fetch, u32 writeback, void* system, u32 value, u32* front, u32* format,
             u32* color_space, u32* width, u32* height);
void interrupt_entry(void);
void normal_entry(void);
void kernel_entry(void);
void audio_entry(void);
uint64_t time_base(void);
uint64_t pcr(void);

enum {
  kMemCommit = 0x1000, kMemReserve = 0x2000, kMemDecommit = 0x4000, kMemRelease = 0x8000, kMemTopDown = 0x100000,
  kMemLargePages = 0x20000000, kMem16MbPages = 0x80000000, kPageReadWrite = 0x04,
  kStatusTimeout = 0x102, kStatusConflictingAddresses = 0xC0000018, kStatusInvalidHandle = 0xC0000008,
  kStatusMutantNotOwned = 0xC0000046, kStatusEndOfFile = 0xC0000011, kStatusAccessDenied = 0xC0000022,
  kStatusObjectNameNotFound = 0xC0000034, kStatusObjectNameCollision = 0xC0000035, kStatusNotFound = 0xC0000225,
  kStatusObjectNameExists = 0x40000000, kStatusObjectTypeMismatch = 0xC0000024, kStatusNotSameDevice = 0xC00000D4,
  kStatusMemoryNotAllocated = 0xC00000A0, kMemFree = 0x10000,
  kStatusObjectPathNotFound = 0xC000003A, kStatusNoMoreFiles = 0x80000006,
  kGenericRead = 0x80000000, kGenericWrite = 0x40000000,
  kOpen = 1, kCreate = 2, kOpenIf = 3, kSync = 0x20, kDirectoryFile = 0x1, kNonDirectoryFile = 0x40,
  kOpened = 1, kCreated = 2,
};

void* memset(void* d, int v, unsigned long n) {
  for (unsigned long k = 0; k < n; k++) ((volatile unsigned char*)d)[k] = (unsigned char)v;
  return d;
}
void* memcpy(void* d, const void* s, unsigned long n) {
  for (unsigned long k = 0; k < n; k++) ((volatile unsigned char*)d)[k] = ((const unsigned char*)s)[k];
  return d;
}

// ANSI_STRING.
struct Ansi {
  uint16_t length, maximum;
  u32 buffer;
};

static int failures;
#define CHECK(cond)                                                          \
  do {                                                                       \
    if (!(cond)) {                                                           \
      failures++;                                                            \
      DbgPrint("FAIL line %d: %s\n", __LINE__, #cond);                       \
    }                                                                        \
  } while (0)

static volatile u32* At(u32 va) { return (volatile u32*)(uintptr_t)va; }
static int Memeq(const void* a, const void* b, unsigned long n) {
  for (unsigned long k = 0; k < n; k++)
    if (((const char*)a)[k] != ((const char*)b)[k]) return 0;
  return 1;
}
static int Streq(const char* a, const char* b) {
  while (*a && *a == *b) a++, b++;
  return *a == *b;
}

// ---- memory -------------------------------------------------------------
static void Memory(void) {
  u32 base = 0, size = 0x3000;
  CHECK(NtAllocateVirtualMemory(&base, &size, kMemReserve | kMemCommit, kPageReadWrite, 0) == 0);
  CHECK(base && base < 0x40000000 && (base & 0xFFF) == 0 && size == 0x3000);
  *At(base + 0x1FFC) = 0x12345678;
  CHECK(*At(base + 0x1FFC) == 0x12345678);
  u32 info[7];
  CHECK(NtQueryVirtualMemory(base + 0x1000, info, 0) == 0);
  CHECK(info[0] == base + 0x1000 && info[1] == base && info[3] == 0x2000 && info[4] == kMemCommit &&
        info[5] == kPageReadWrite);
  // Decommitted pages come back zero.
  u32 b2 = base + 0x1000, s2 = 0x1000;
  CHECK(NtFreeVirtualMemory(&b2, &s2, kMemDecommit, 0) == 0);
  CHECK(NtQueryVirtualMemory(base + 0x1000, info, 0) == 0 && info[4] == kMemReserve && info[3] == 0x1000);
  b2 = base + 0x1000;
  s2 = 0x1000;
  CHECK(NtAllocateVirtualMemory(&b2, &s2, kMemCommit, kPageReadWrite, 0) == 0 && b2 == base + 0x1000);
  CHECK(*At(base + 0x1FFC) == 0);
  // A fixed reservation over one that exists fails.
  u32 b3 = base, s3 = 0x1000;
  CHECK(NtAllocateVirtualMemory(&b3, &s3, kMemReserve, kPageReadWrite, 0) == kStatusConflictingAddresses);
  u32 rel = 0;
  CHECK(NtFreeVirtualMemory(&base, &rel, kMemRelease, 0) == 0 && rel == 0x3000);
  // A commit with no base reserves too (top down here, as a title's heap
  // asks); none reaches the never-mapped first 64 KiB.
  base = 0;
  size = 0xFFFC;
  CHECK(NtAllocateVirtualMemory(&base, &size, kMemCommit | kMemTopDown, kPageReadWrite, 0) == 0);
  CHECK(base >= 0x10000 && base < 0x40000000 && size == 0x10000);
  *At(base + 0xFFF8) = 0x55AA55AA;
  CHECK(*At(base + 0xFFF8) == 0x55AA55AA);
  rel = 0;
  CHECK(NtFreeVirtualMemory(&base, &rel, kMemRelease, 0) == 0 && rel == 0x10000);
  b2 = 0x1000;
  s2 = 0x1000;
  CHECK(NtAllocateVirtualMemory(&b2, &s2, kMemCommit, kPageReadWrite, 0) != 0);
  // Large pages: 64 KiB granularity from 0x40000000.
  base = 0;
  size = 0x1000;
  CHECK(NtAllocateVirtualMemory(&base, &size, kMemReserve | kMemCommit | kMemLargePages, kPageReadWrite, 0) == 0);
  CHECK(base >= 0x40000000 && base < 0x80000000 && (base & 0xFFFF) == 0 && size == 0x10000);
  rel = 0;
  CHECK(NtFreeVirtualMemory(&base, &rel, kMemRelease, 0) == 0);
  // ... and commits and decommits whole 64 KiB pages in it, as a heap
  // that grows by the sizes it gets back relies on.
  base = 0;
  size = 0x100000;
  CHECK(NtAllocateVirtualMemory(&base, &size, kMemReserve | kMemLargePages, kPageReadWrite, 0) == 0);
  b2 = base;
  s2 = 0x20E50;
  CHECK(NtAllocateVirtualMemory(&b2, &s2, kMemCommit | kMemLargePages, kPageReadWrite, 0) == 0 && b2 == base &&
        s2 == 0x30000);
  b2 = base + 0x10100;
  s2 = 0x100;
  CHECK(NtFreeVirtualMemory(&b2, &s2, kMemDecommit, 0) == 0 && b2 == base + 0x10000 && s2 == 0x10000);
  CHECK(NtQueryVirtualMemory(base + 0x10000, info, 0) == 0 && info[4] == kMemReserve && info[3] == 0x10000);
  rel = 0;
  CHECK(NtFreeVirtualMemory(&base, &rel, kMemRelease, 0) == 0 && rel == 0x100000);
  // A release with a size frees that range of one reservation: a tail (as
  // a heap shrinking a block in place gives back), then a middle, whose
  // remainder becomes a reservation of its own.
  base = 0;
  size = 0x6000;
  CHECK(NtAllocateVirtualMemory(&base, &size, kMemReserve | kMemCommit, kPageReadWrite, 0) == 0);
  *At(base + 0x5000) = 1;
  b2 = base + 0x4100;
  s2 = 0x1F00;
  CHECK(NtFreeVirtualMemory(&b2, &s2, kMemRelease, 0) == 0 && b2 == base + 0x4000 && s2 == 0x2000);
  CHECK(NtQueryVirtualMemory(base + 0x4000, info, 0) == 0 && info[4] == kMemFree);
  b2 = base + 0x1000;
  s2 = 0x1000;
  CHECK(NtFreeVirtualMemory(&b2, &s2, kMemRelease, 0) == 0);
  CHECK(NtQueryVirtualMemory(base + 0x2000, info, 0) == 0 && info[1] == base + 0x2000 && info[3] == 0x2000 &&
        info[4] == kMemCommit && info[2] == kPageReadWrite);
  rel = 0;
  CHECK(NtFreeVirtualMemory(&base, &rel, kMemRelease, 0) == 0 && rel == 0x1000);
  b2 = base + 0x2000;
  rel = 0;
  CHECK(NtFreeVirtualMemory(&b2, &rel, kMemRelease, 0) == 0 && rel == 0x2000);
  b2 = base + 0x5000;
  s2 = 0x1000;
  CHECK(NtFreeVirtualMemory(&b2, &s2, kMemRelease, 0) == kStatusMemoryNotAllocated);
  // The kernel pool.
  u32 p = ExAllocatePool(100);
  CHECK(p != 0 && (p & 15) == 0);
  ExFreePool(p);
}

// ---- physical memory and its views ---------------------------------------------
static void Physical(void) {
  u32 small = MmAllocatePhysicalMemoryEx(0, 0x2000, kPageReadWrite, 0, 0, 0x1000);
  u32 large = MmAllocatePhysicalMemoryEx(0, 0x10000, kPageReadWrite | kMemLargePages, 0, 0, 0x10000);
  u32 huge = MmAllocatePhysicalMemoryEx(0, 0x1000000, kPageReadWrite | kMem16MbPages, 0, 0x1FFFFFFF, 0x1000000);
  CHECK(small >= 0xE0000000 && large >= 0xA0000000 && large < 0xC0000000 && huge >= 0xC0000000 &&
        huge < 0xE0000000);
  // The 4 KiB view is one page into physical memory: what is written
  // there is read through the 64 KiB view at its physical address.
  u32 pa = MmGetPhysicalAddress(small);
  CHECK(pa == small - 0xE0000000 + 0x1000);
  *At(small + 4) = 0xCAFEF00D;
  CHECK(*At(0xA0000000 + pa + 4) == 0xCAFEF00D);
  CHECK(*At(0xC0000000 + pa + 4) == 0xCAFEF00D);
  CHECK(MmGetPhysicalAddress(large) == large - 0xA0000000);
  CHECK(MmQueryAddressProtect(large) == kPageReadWrite);
  // No two allocations share physical pages, whatever their views.
  u32 pl = MmGetPhysicalAddress(large), ph = MmGetPhysicalAddress(huge);
  CHECK(pa + 0x2000 <= pl || pl + 0x10000 <= pa);
  CHECK(pa + 0x2000 <= ph || ph + 0x1000000 <= pa);
  CHECK(pl + 0x10000 <= ph || ph + 0x1000000 <= pl);
  // Physical bounds.
  u32 low = MmAllocatePhysicalMemoryEx(0, 0x10000, kPageReadWrite | kMemLargePages, 0x100000, 0x1FFFFF, 0x10000);
  CHECK(low && MmGetPhysicalAddress(low) >= 0x100000 && MmGetPhysicalAddress(low) + 0x10000 <= 0x200000);
  MmFreePhysicalMemory(0, low);
  // Whole pages of the view are committed.
  u32 part = MmAllocatePhysicalMemoryEx(0, 0x1000, kPageReadWrite | kMemLargePages, 0, 0, 0x10000), info[7];
  CHECK(part && NtQueryVirtualMemory(part + 0x8000, info, 0) == 0 && info[4] == kMemCommit);
  MmFreePhysicalMemory(0, part);
  MmFreePhysicalMemory(0, small);
  MmFreePhysicalMemory(0, large);
  MmFreePhysicalMemory(0, huge);
}

// ---- threads, TLS, synchronisation ------------------------------------------
struct Shared {
  u32 cs[7];      // RTL_CRITICAL_SECTION
  u32 event[4];   // a synchronization event (KEVENT)
  u32 count;
  u32 tls_index;
  u32 tls_seen;   // what a worker read from its own slot before setting it
  u32 kthread;    // a worker's KPCR+0x100
  u32 thread_id;  // a worker's KTHREAD+0x14C
  u32 cpu[2];     // a worker's KPCR+0x10C and KTHREAD+0xBF
  u32 handle;     // a worker's own handle
  u32 stage;
  u32 ticks, stop;
};
static struct Shared shared;

static int WaitFor(volatile u32* p, u32 v) {
  int64_t ms = -10000;
  for (int k = 0; k < 1000 && *p != v; k++) KeDelayExecutionThread(1, 0, &ms);
  return *p == v;
}

static u32 Counter(void* arg) {
  (void)arg;
  for (int k = 0; k < 20000; k++) {
    RtlEnterCriticalSection(shared.cs);
    RtlEnterCriticalSection(shared.cs);  // recursion
    u32 v = shared.count;
    shared.count = v + 1;
    RtlLeaveCriticalSection(shared.cs);
    RtlLeaveCriticalSection(shared.cs);
  }
  return 7;
}

static u32 Probe(void* arg) {
  shared.tls_seen = KeTlsGetValue(shared.tls_index);
  KeTlsSetValue(shared.tls_index, 0x2222);
  shared.kthread = *At((u32)pcr() + 0x100);
  shared.thread_id = *At(shared.kthread + 0x14C);
  KeWaitForSingleObject(shared.event, 3, 1, 0, 0);
  return (u32)(uintptr_t)arg;
}

static u32 Affine(void* arg) {
  (void)arg;
  u32 pc = (u32)pcr();
  shared.cpu[0] = *(volatile unsigned char*)(uintptr_t)(pc + 0x10C);
  shared.cpu[1] = *(volatile unsigned char*)(uintptr_t)(*At(pc + 0x100) + 0xBF);
  return 0;
}

static u32 SelfSuspend(void* arg) {
  (void)arg;
  u32 previous = 9;
  shared.stage = 1;
  u32 status = NtSuspendThread(shared.handle, &previous);
  shared.stage = 2;
  return status == 0 && previous == 0 ? 5 : 6;
}

static u32 Ticker(void* arg) {
  (void)arg;
  int64_t ms = -10000;
  while (!*(volatile u32*)&shared.stop) {
    shared.ticks++;
    KeDelayExecutionThread(1, 0, &ms);
  }
  return 0;
}

// Runs lifted code, calling into the kernel (an import) on each round.
static u32 Spinner(void* arg) {
  (void)arg;
  while (!*(volatile u32*)&shared.stop) {
    shared.ticks++;
    KeGetCurrentProcessType();
  }
  return 0;
}

// Waits on a KEVENT for up to a second and counts its result: satisfied
// in handed[0], anything else in handed[1].
static volatile u32 handed[2];
static u32 EventWaiter(void* event) {
  int64_t second = -10000000;
  u32 status = KeWaitForSingleObject(event, 3, 1, 0, &second);
  __sync_fetch_and_add(&handed[status == 0 ? 0 : 1], 1);
  return status;
}

static u32 Quick(void* arg) { return (u32)(uintptr_t)arg; }

static u32 Setter(void* arg) {
  int64_t ms = -200000;
  KeDelayExecutionThread(1, 0, &ms);
  KeSetEvent(arg, 1, 0);
  return 0;
}

static void Threads(void) {
  CHECK(KeGetCurrentProcessType() == 1);
  // r13: the KPCR, whose current thread is this thread's KTHREAD, a
  // thread object.
  u32 me = *At((u32)pcr() + 0x100);
  CHECK(me != 0 && (*At(me) >> 24) == 6);

  // TLS: a slot per thread.
  shared.tls_index = KeTlsAlloc();
  CHECK(shared.tls_index != 0xFFFFFFFF);
  CHECK(KeTlsSetValue(shared.tls_index, 0x1111) && KeTlsGetValue(shared.tls_index) == 0x1111);
  // A slot freed and handed out again reads 0.
  u32 slot = KeTlsAlloc();
  CHECK(KeTlsSetValue(slot, 5) && KeTlsFree(slot) && KeTlsAlloc() == slot && KeTlsGetValue(slot) == 0);
  KeTlsFree(slot);

  // A worker created suspended, started, waiting on an event.
  shared.event[0] = 1 << 24;  // KeInitializeEvent: a synchronization event, not signalled
  shared.event[1] = 0;
  shared.event[2] = shared.event[3] = (u32)(uintptr_t)&shared.event[2];
  u32 h = 0, id = 0;
  CHECK(ExCreateThread(&h, 0, &id, (void*)thread_startup, (void*)Probe, (void*)(uintptr_t)42, 1) == 0);
  CHECK(h != 0 && id != 0);
  int64_t ten_ms = -100000;
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, &ten_ms) == kStatusTimeout);  // still suspended
  u32 previous = 9;
  CHECK(NtResumeThread(h, &previous) == 0 && previous == 1);
  int64_t zero = 0;
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, &zero) == kStatusTimeout);    // waiting on the event
  KeDelayExecutionThread(1, 0, &ten_ms);
  KeSetEvent(shared.event, 1, 0);
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0);
  CHECK(shared.event[1] == 0);  // the waiter took the synchronization event
  CHECK(shared.tls_seen == 0 && KeTlsGetValue(shared.tls_index) == 0x1111);
  CHECK(shared.kthread != me && shared.thread_id == id);
  // The thread's exit status, and its KTHREAD through its handle.
  u32 obj = 0;
  CHECK(ObReferenceObjectByHandle(h, __imp_ExThreadObjectType, &obj) == 0 && obj == shared.kthread);
  CHECK(*At(obj + 0x140) == 42);
  ObDereferenceObject(obj);
  CHECK(NtClose(h) == 0 && NtClose(h) == kStatusInvalidHandle);
  // Flag 0x80: the KTHREAD in place of a handle.
  u32 kt = 0;
  CHECK(ExCreateThread(&kt, 0, &id, (void*)thread_startup, (void*)Probe, (void*)(uintptr_t)43, 0x80) == 0);
  CHECK(kt != 0 && (*At(kt) >> 24) == 6 && *At(kt + 0x14C) == id);
  KeDelayExecutionThread(1, 0, &ten_ms);
  KeSetEvent(shared.event, 1, 0);
  CHECK(KeWaitForSingleObject((void*)(uintptr_t)kt, 3, 1, 0, 0) == 0 && shared.kthread == kt);
  CHECK(*At(kt + 0x140) == 43);
  ObDereferenceObject(kt);

  // Bits 24-31 of the flags: an affinity mask, the thread on its lowest
  // processor from its first instruction.
  CHECK(ExCreateThread(&h, 0, 0, (void*)thread_startup, (void*)Affine, 0, 0x0C000000) == 0);
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0 && shared.cpu[0] == 2 && shared.cpu[1] == 2);
  NtClose(h);
  // A thread suspending itself stops in the call until resumed, which
  // returns the count it had.
  CHECK(ExCreateThread(&h, 0, 0, (void*)thread_startup, (void*)SelfSuspend, 0, 1) == 0);
  shared.handle = h;
  CHECK(NtResumeThread(h, &previous) == 0 && previous == 1);
  CHECK(WaitFor(&shared.stage, 1));
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, &ten_ms) == kStatusTimeout && shared.stage == 1);
  CHECK(NtResumeThread(h, &previous) == 0 && previous == 1);
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0 && shared.stage == 2);
  CHECK(ObReferenceObjectByHandle(h, __imp_ExThreadObjectType, &obj) == 0 && *At(obj + 0x140) == 5);
  ObDereferenceObject(obj);
  NtClose(h);
  // Another thread running lifted code has stopped when NtSuspendThread
  // returns (at its next call into the kernel); one in a delay stops in it.
  for (int k = 0; k < 2; k++) {
    shared.stop = shared.ticks = 0;
    CHECK(ExCreateThread(&h, 0, 0, (void*)thread_startup, k ? (void*)Ticker : (void*)Spinner, 0, 0) == 0);
    KeDelayExecutionThread(1, 0, &ten_ms);
    CHECK(NtSuspendThread(h, &previous) == 0 && previous == 0);
    u32 held = shared.ticks;
    for (int j = 0; j < 3; j++) KeDelayExecutionThread(1, 0, &ten_ms);
    CHECK(held != 0 && shared.ticks == held);
    shared.stop = 1;
    CHECK(NtResumeThread(h, &previous) == 0 && previous == 1);
    CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0);
    NtClose(h);
  }
  // A suspended thread is out of its wait: a set meanwhile goes to another
  // waiter; resumed, it waits again.
  u32 sync[4] = {1 << 24, 0, (u32)(uintptr_t)&sync[2], (u32)(uintptr_t)&sync[2]};
  handed[0] = handed[1] = 0;
  CHECK(ExCreateThread(&h, 0, 0, (void*)thread_startup, (void*)EventWaiter, sync, 0) == 0);
  KeDelayExecutionThread(1, 0, &ten_ms);
  CHECK(NtSuspendThread(h, &previous) == 0 && previous == 0);
  KeSetEvent(sync, 1, 0);
  CHECK(KeWaitForSingleObject(sync, 3, 1, 0, &zero) == 0);
  CHECK(NtResumeThread(h, &previous) == 0 && previous == 1);
  KeDelayExecutionThread(1, 0, &ten_ms);
  CHECK(handed[0] == 0 && handed[1] == 0);
  KeSetEvent(sync, 1, 0);
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0 && handed[0] == 1 && sync[1] == 0);
  NtClose(h);
  // A thread whose start routine returns at once: its handle and id are
  // still its own (its creator holds it until they are written).
  for (u32 k = 0; k < 64; k++) {
    CHECK(ExCreateThread(&h, 0, &id, (void*)thread_startup, (void*)Quick, (void*)(uintptr_t)k, 0) == 0);
    CHECK(ObReferenceObjectByHandle(h, __imp_ExThreadObjectType, &obj) == 0 && *At(obj + 0x14C) == id);
    CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0 && *At(obj + 0x140) == k);
    ObDereferenceObject(obj);
    NtClose(h);
  }

  // A relative timeout of 0x8000000000000000 (XAPI's INFINITE) waits
  // until the object is signalled.
  u32 ev[4] = {0, 0, (u32)(uintptr_t)&ev[2], (u32)(uintptr_t)&ev[2]};  // a notification event
  int64_t forever = (int64_t)0x8000000000000000ull;
  CHECK(ExCreateThread(&h, 0, 0, (void*)thread_startup, (void*)Setter, ev, 0) == 0);
  CHECK(KeWaitForSingleObject(ev, 3, 1, 0, &forever) == 0 && ev[1] == 1);
  NtWaitForSingleObjectEx(h, 1, 0, 0);
  NtClose(h);

  // Four threads contending for a critical section.
  RtlInitializeCriticalSection(shared.cs);
  u32 hs[4];
  for (int k = 0; k < 4; k++) CHECK(ExCreateThread(&hs[k], 0x10000, 0, (void*)thread_startup, (void*)Counter, 0, 0) == 0);
  CHECK(NtWaitForMultipleObjectsEx(4, hs, 0, 1, 0, 0) == 0);
  CHECK(shared.count == 80000);
  CHECK(shared.cs[4] == 0xFFFFFFFF && shared.cs[6] == 0);  // free again
  for (int k = 0; k < 4; k++) NtClose(hs[k]);
  CHECK(RtlTryEnterCriticalSection(shared.cs) == 1 && shared.cs[6] == me);
  RtlLeaveCriticalSection(shared.cs);
}

static void Objects(void) {
  // A notification event created signalled stays signalled through waits.
  u32 e = 0;
  CHECK(NtCreateEvent(&e, 0, 0, 1) == 0);
  int64_t zero = 0;
  CHECK(NtWaitForSingleObjectEx(e, 1, 0, &zero) == 0 && NtWaitForSingleObjectEx(e, 1, 0, &zero) == 0);
  // A synchronization event is taken by one wait.
  u32 s = 0, previous = 9;
  CHECK(NtCreateEvent(&s, 0, 1, 0) == 0);
  CHECK(NtWaitForSingleObjectEx(s, 1, 0, &zero) == kStatusTimeout);
  CHECK(NtSetEvent(s, &previous) == 0 && previous == 0);
  u32 both[2] = {s, e};
  CHECK(NtWaitForMultipleObjectsEx(2, both, 1, 1, 0, &zero) == 0);  // any: the first, s
  CHECK(NtWaitForMultipleObjectsEx(2, both, 1, 1, 0, &zero) == 1);  // s taken: e
  CHECK(NtWaitForMultipleObjectsEx(2, both, 0, 1, 0, &zero) == kStatusTimeout);  // all
  NtClose(s);
  NtClose(e);
  // A semaphore counts down with waits.
  u32 sem[5];
  KeInitializeSemaphore(sem, 2, 3);
  CHECK(KeWaitForSingleObject(sem, 3, 1, 0, &zero) == 0 && KeWaitForSingleObject(sem, 3, 1, 0, &zero) == 0);
  CHECK(KeWaitForSingleObject(sem, 3, 1, 0, &zero) == kStatusTimeout);
  CHECK(KeReleaseSemaphore(sem, 1, 3, 0) == 0 && sem[1] == 3);
  // A mutant is recursive for its owner.
  u32 m = 0;
  CHECK(NtCreateMutant(&m, 0, 1) == 0);
  CHECK(NtWaitForSingleObjectEx(m, 1, 0, &zero) == 0);
  CHECK(NtReleaseMutant(m, &previous) == 0 && NtReleaseMutant(m, &previous) == 0 && previous == 0);
  CHECK(NtReleaseMutant(m, &previous) == kStatusMutantNotOwned);
  NtClose(m);
  // Named objects, as XAPI's CreateEvent and CreateMutex ask for them:
  // under \BaseNamedObjects (-4) with OBJ_OPENIF.
  struct Ansi name;
  RtlInitAnsiString(&name, "Shared");
  u32 attributes[3] = {0xFFFFFFFC, (u32)(uintptr_t)&name, 0x80};
  u32 n1 = 0, n2 = 0;
  CHECK(NtCreateEvent(&n1, attributes, 0, 0) == 0);
  CHECK(NtCreateEvent(&n2, attributes, 1, 1) == kStatusObjectNameExists && n2 && n2 != n1);
  CHECK(NtWaitForSingleObjectEx(n1, 1, 0, &zero) == kStatusTimeout);  // the first one's state
  CHECK(NtSetEvent(n2, &previous) == 0 && NtWaitForSingleObjectEx(n1, 1, 0, &zero) == 0);
  CHECK(NtCreateMutant(&m, attributes, 0) == kStatusObjectTypeMismatch);
  attributes[2] = 0;
  CHECK(NtCreateEvent(&s, attributes, 0, 0) == kStatusObjectNameCollision);
  RtlInitAnsiString(&name, "SHARED");
  CHECK(NtCreateEvent(&s, attributes, 0, 0) == 0);  // names compare exactly
  NtClose(s);
  attributes[2] = 0xC0;  // ... unless case-insensitive
  CHECK(NtCreateEvent(&s, attributes, 0, 0) == kStatusObjectNameExists);
  NtClose(s);
  NtClose(n1);
  NtClose(n2);
  attributes[2] = 0x80;  // the name went with its last handle
  CHECK(NtCreateMutant(&m, attributes, 0) == 0);
  NtClose(m);
  // A spin lock holds the owner's KPCR.
  u32 lock = 0;
  u32 irql = KfAcquireSpinLock(&lock);
  CHECK(lock == (u32)pcr() && irql == 0);
  KfReleaseSpinLock(&lock, irql);
  CHECK(lock == 0);
  // A release frees the lock whoever holds it.
  lock = 0x1234;
  KfReleaseSpinLock(&lock, irql);
  CHECK(lock == 0);
  // An interlocked list: pop and flush.
  u32 nodes[3] = {(u32)(uintptr_t)&nodes[1], (u32)(uintptr_t)&nodes[2], 0};
  u32 head[2] = {(u32)(uintptr_t)&nodes[0], 3 << 16};
  CHECK(InterlockedPopEntrySList(head) == (u32)(uintptr_t)&nodes[0]);
  CHECK(head[0] == (u32)(uintptr_t)&nodes[1] && (head[1] >> 16) == 2);
  CHECK(InterlockedFlushSList(head) == (u32)(uintptr_t)&nodes[1] && head[0] == 0 && (head[1] >> 16) == 0);
  // A call through an import's slot reaches its handler.
  u32 (*close)(u32) = (u32(*)(u32))(uintptr_t)__imp_NtClose;
  CHECK(close(0x1234) == kStatusInvalidHandle);
}

// ---- strings and formatting, variables, time ------------------------------------------
static u32 Vsn(char* buf, u32 count, const char* fmt, ...) {
  // The guest's va_list: the 8-byte slots of the parameter area.
  __builtin_va_list ap;
  __builtin_va_start(ap, fmt);
  u32 n = _vsnprintf(buf, count, fmt, (uint64_t*)ap);
  __builtin_va_end(ap);
  return n;
}

static void Strings(void) {
  static const char kName[] = "kernel";
  u32 s[2];
  RtlInitAnsiString(s, kName);
  CHECK(s[0] == (6u << 16 | 7) && s[1] == (u32)(uintptr_t)kName);
  char buf[96];
  u32 n = sprintf(buf, "%d|%5x|%-4s|%08X|%I64d|%.3f", -12, 0xAB, "ab", 0xBEEF, (int64_t)-5000000000, 2.5);
  CHECK(Streq(buf, "-12|   ab|ab  |0000BEEF|-5000000000|2.500") && n == 41);
  static const uint16_t kWide[] = {'w', 'i', 0};
  CHECK(sprintf(buf, "%c|%%|%S|%ls|%-3d|", 'z', kWide, kWide, 7) == 14 && Streq(buf, "z|%|wi|wi|7  |"));
  // The ninth argument comes from the caller's parameter area.
  CHECK(sprintf9(buf, "%c%c%c%c%c%c%c", 'a', 'b', 'c', 'd', 'e', 'f', 'g') == 7 && Streq(buf, "abcdefg"));
  CHECK(Vsn(buf, sizeof buf, "%s=%u", "n", 7u) == 3 && Streq(buf, "n=7"));
  CHECK(Vsn(buf, 4, "%s", "toolong") == 0xFFFFFFFF);
  u32 words[4] = {0, 0, 0, 0};
  RtlFillMemoryUlong(words, 12, 0x01020304);
  CHECK(words[0] == 0x01020304 && words[2] == 0x01020304 && words[3] == 0);
}

static void Variables(void) {
  const uint16_t* version = (const uint16_t*)(uintptr_t)__imp_XboxKrnlVersion;
  CHECK(version[0] == 2 && version[2] == 17559);
  // The time stamp bundle's tick count follows the time.
  volatile u32* bundle = At(__imp_KeTimeStampBundle);
  u32 before = bundle[4];
  int64_t fifty_ms = -500000;
  KeDelayExecutionThread(1, 0, &fifty_ms);
  u32 after = bundle[4];
  CHECK(after - before >= 40 && after - before < 1000);
  uint64_t t0 = time_base();
  KeDelayExecutionThread(1, 0, &fifty_ms);
  uint64_t t1 = time_base(), f = KeQueryPerformanceFrequency();
  CHECK(f == 49875000 && t1 - t0 >= f / 25 && t1 - t0 < f);
  uint64_t now = 0;
  KeQuerySystemTime(&now);
  CHECK(now > 0x01D0000000000000ull);  // after 2014
}


// ---- files ------------------------------------------------------------------
// The title's disc (tests/CMakeLists.txt writes it): data.bin (the 32 bytes
// 0-9A-V), Sub/Name.TXT ("hello"), Sub/other.dat.
struct Attributes {
  u32 root, name, attributes;
};

static u32 OpenPath(u32* h, const char* path, u32 access, u32 disposition, u32 options, u32* io) {
  struct Ansi name;
  RtlInitAnsiString(&name, path);
  struct Attributes a = {0, (u32)(uintptr_t)&name, 0};
  return NtCreateFile9(h, access, &a, io, 0, 0x80, 7, disposition, options);
}

static u32 QueryFullIn(u32 root, const char* path, uint64_t* info) {
  struct Ansi name;
  RtlInitAnsiString(&name, path);
  struct Attributes a = {root, (u32)(uintptr_t)&name, 0x40};
  return NtQueryFullAttributesFile(&a, info);
}
static u32 QueryFull(const char* path, uint64_t* info) { return QueryFullIn(0, path, info); }

static void Files(void) {
  u32 h = 0, d = 0, io[2];
  char buf[64];
  uint64_t info[7];
  // A file on the disc, read in order, then at an offset.
  CHECK(OpenPath(&h, "game:\\data.bin", kGenericRead, kOpen, kSync | kNonDirectoryFile, io) == 0 &&
        io[1] == kOpened);
  CHECK(NtReadFile(h, 0, 0, 0, io, buf, 10, 0) == 0 && io[1] == 10 && Memeq(buf, "0123456789", 10));
  CHECK(NtReadFile(h, 0, 0, 0, io, buf, 64, 0) == 0 && io[1] == 22 && buf[0] == 'A' && buf[21] == 'V');
  CHECK(NtReadFile(h, 0, 0, 0, io, buf, 64, 0) == kStatusEndOfFile && io[1] == 0);
  CHECK(NtQueryInformationFile(h, io, info, 8, 14) == 0 && info[0] == 32);
  int64_t at = 4;
  CHECK(NtReadFile(h, 0, 0, 0, io, buf, 2, &at) == 0 && Memeq(buf, "45", 2));
  CHECK(NtQueryInformationFile(h, io, info, 0x38, 34) == 0 && info[5] == 32 && (info[6] >> 32) == 0x01);
  u32 volume[8];
  CHECK(NtQueryVolumeInformationFile(h, io, volume, sizeof volume, 5) == 0 && volume[2] == 4 &&
        Memeq(&volume[3], "GDFX", 4));
  NtClose(h);
  // Names are case-insensitive; the disc is read-only.
  CHECK(OpenPath(&h, "GAME:\\SUB\\name.txt", kGenericRead, kOpen, kSync, io) == 0);
  CHECK(NtReadFile(h, 0, 0, 0, io, buf, 64, 0) == 0 && io[1] == 5 && Memeq(buf, "hello", 5));
  NtClose(h);
  CHECK(OpenPath(&h, "d:\\data.bin", kGenericWrite, kOpen, kSync, io) == kStatusAccessDenied);
  CHECK(OpenPath(&h, "game:\\missing.bin", kGenericRead, kOpen, kSync, io) == kStatusObjectNameNotFound);
  CHECK(OpenPath(&h, "game:\\nodir\\x.bin", kGenericRead, kOpen, kSync, io) == kStatusObjectPathNotFound);
  // A backslash after a name names a directory: a file's is invalid, a
  // directory's opens (and is a directory to a file open).
  CHECK(OpenPath(&h, "game:\\data.bin\\", kGenericRead, kOpen, kSync, io) == 0xC0000033);
  CHECK(OpenPath(&h, "game:\\Sub\\", kGenericRead, kOpen, kSync | kNonDirectoryFile, io) == 0xC00000BA);
  CHECK(OpenPath(&d, "game:\\Sub\\", kGenericRead, kOpen, kSync | kDirectoryFile, io) == 0);
  NtClose(d);
  // A directory's entries, through a mask, one per query.
  u32 entry[32];
  struct Ansi mask;
  RtlInitAnsiString(&mask, "*.txt");
  CHECK(OpenPath(&d, "game:\\Sub", kGenericRead, kOpen, kSync | kDirectoryFile, io) == 0);
  CHECK(NtQueryDirectoryFile9(d, 0, 0, 0, io, entry, sizeof entry, &mask, 0) == 0 && entry[15] == 8 &&
        Memeq(&entry[16], "Name.TXT", 8) && entry[11] == 5);
  CHECK(NtQueryDirectoryFile9(d, 0, 0, 0, io, entry, sizeof entry, 0, 0) == kStatusNoMoreFiles);
  CHECK(NtQueryDirectoryFile9(d, 0, 0, 0, io, entry, sizeof entry, 0, 1) == 0 && Memeq(&entry[16], "Name.TXT", 8));
  CHECK(NtQueryDirectoryFile9(d, 0, 0, 0, io, entry, sizeof entry, 0, 0) == 0 && entry[15] == 9 &&
        Memeq(&entry[16], "other.dat", 9));
  NtClose(d);

  // Writable storage, through a link the title makes.
  struct Ansi link, target;
  RtlInitAnsiString(&link, "\\??\\hdd:");
  RtlInitAnsiString(&target, "\\Device\\Harddisk0\\Partition1");
  CHECK(ObCreateSymbolicLink(&link, &target) == 0);
  CHECK(OpenPath(&d, "hdd:\\dir", kGenericRead, kCreate, kSync | kDirectoryFile, io) == 0 && io[1] == kCreated);
  CHECK(OpenPath(&h, "hdd:\\dir\\f.bin", kGenericRead | kGenericWrite, kCreate, kSync | kNonDirectoryFile, io) == 0);
  CHECK(OpenPath(&h, "hdd:\\dir\\f.bin", kGenericRead, kCreate, kSync, io) == kStatusObjectNameCollision);
  CHECK(NtWriteFile(h, 0, 0, 0, io, "abcdefghij", 10, 0) == 0 && io[1] == 10);
  uint64_t v = 0;
  CHECK(NtSetInformationFile(h, io, &v, 8, 14) == 0);
  CHECK(NtReadFile(h, 0, 0, 0, io, buf, 64, 0) == 0 && io[1] == 10 && Memeq(buf, "abcdefghij", 10));
  v = 4;
  CHECK(NtSetInformationFile(h, io, &v, 8, 20) == 0);
  CHECK(NtFlushBuffersFile(h, io) == 0);
  // A directory with entries is not deleted; a file to create with a
  // backslash after its name is invalid.
  u32 probe = 0;
  uint8_t remove_dir = 1;
  CHECK(OpenPath(&probe, "hdd:\\dir", kGenericRead, kOpen, kSync | kDirectoryFile | 0x1000, io) == 0xC0000101);
  CHECK(NtSetInformationFile(d, io, &remove_dir, 1, 13) == 0xC0000101);
  CHECK(OpenPath(&probe, "hdd:\\dir\\new\\", kGenericWrite, kCreate, kSync | kNonDirectoryFile, io) == 0xC0000033);
  // XAPI's root: the DOS devices directory (ObDosDevices, -3), as xam's
  // GetFileAttributesExA passes it.
  CHECK(QueryFullIn(0xFFFFFFFD, "D:\\data.bin", info) == 0 && info[5] == 32);
  CHECK(QueryFull("hdd:\\DIR\\F.BIN", info) == 0 && info[5] == 4 && (info[6] >> 32) == 0x80);
  // An asynchronous read completes with its event set.
  u32 e = 0;
  CHECK(NtCreateEvent(&e, 0, 0, 0) == 0);
  at = 1;
  CHECK(NtReadFile(h, e, 0, 0, io, buf, 2, &at) == 0 && io[0] == 0 && io[1] == 2 && Memeq(buf, "bc", 2));
  int64_t zero = 0;
  CHECK(NtWaitForSingleObjectEx(e, 1, 0, &zero) == 0);
  NtClose(e);
  // Renamed, then deleted when its handle closes.
  uint8_t remove_now = 1;
  struct {
    u32 replace, root;
    struct Ansi name;
  } rename = {0, 0, {0, 0, 0}};
  RtlInitAnsiString(&rename.name, "hdd:\\dir\\g.bin");
  CHECK(NtSetInformationFile(h, io, &rename, sizeof rename, 10) == 0);
  // Replace-if-exists is a BOOLEAN: XAPI's MoveFile leaves the padding
  // after it unset.
  u32 other = 0;
  CHECK(OpenPath(&other, "hdd:\\dir\\o.bin", kGenericRead | kGenericWrite, kCreate, kSync | kNonDirectoryFile, io) == 0);
  RtlInitAnsiString(&rename.name, "hdd:\\dir\\g.bin");
  rename.replace = 0x00FFFFFF;
  CHECK(NtSetInformationFile(other, io, &rename, sizeof rename, 10) == kStatusObjectNameCollision);
  RtlInitAnsiString(&rename.name, "game:\\o.bin");
  CHECK(NtSetInformationFile(other, io, &rename, sizeof rename, 10) == kStatusNotSameDevice);
  CHECK(NtSetInformationFile(other, io, &remove_now, 1, 13) == 0);
  NtClose(other);
  rename.replace = 0;
  RtlInitAnsiString(&rename.name, "hdd:\\dir\\g.bin");
  CHECK(QueryFull("hdd:\\dir\\f.bin", info) == kStatusObjectNameNotFound && QueryFull("hdd:\\dir\\g.bin", info) == 0);
  CHECK(NtSetInformationFile(h, io, &remove_now, 1, 13) == 0);
  NtClose(h);
  CHECK(QueryFull("hdd:\\dir\\g.bin", info) == kStatusObjectNameNotFound);
  CHECK(NtQueryVolumeInformationFile(d, io, volume, sizeof volume, 5) == 0 && Memeq(&volume[3], "FATX", 4));
  NtClose(d);
  CHECK(ObDeleteSymbolicLink(&link) == 0);
  CHECK(OpenPath(&d, "hdd:\\dir", kGenericRead, kOpen, kSync | kDirectoryFile, io) == kStatusObjectPathNotFound);
  // Opened through NtOpenFile, by the device's own name.
  struct Ansi name;
  RtlInitAnsiString(&name, "\\Device\\Harddisk0\\Partition1\\dir");
  struct Attributes a = {0, (u32)(uintptr_t)&name, 0};
  CHECK(NtOpenFile(&d, kGenericRead, &a, io, 7, kSync | kDirectoryFile) == 0 && io[1] == kOpened);
  NtClose(d);
}

// ---- the title's module -----------------------------------------------------
// tests/CMakeLists.txt gives the title's XEX the system flags 0x400 and a
// title workspace size (header 0x00020401) of 0x123000, and runs it as
// title.xex; xbox360_elf2xex makes this its resource "Strings".
const char __xex_resource_Strings[] __attribute__((used)) = "resource data";

static void Modules(void) {
  u32 h = *At(__imp_XexExecutableModuleHandle);
  CHECK(h != 0);
  if (!h) return;
  u32 headers = At(h)[0x58 / 4], start = (u32)(uintptr_t)_start;
  CHECK(Memeq((const void*)(uintptr_t)headers, "XEX2", 4));
  CHECK(At(h)[0x1C / 4] == 0x82000000 && At(h)[0x3C / 4] == start);
  // Its base name, in UTF-16.
  CHECK((At(h)[0x2C / 4] >> 16) == 18 && *(volatile uint16_t*)(uintptr_t)At(h)[0x30 / 4] == 't');
  // Header fields: a value in the table, a dword's address, a missing key.
  CHECK(RtlImageXexHeaderField(headers, 0x00010100) == start);
  u32 workspace = RtlImageXexHeaderField(headers, 0x00020401);
  CHECK(workspace && *At(workspace) == 0x123000);
  CHECK(RtlImageXexHeaderField(headers, 0x00040006) == 0);
  CHECK(XexCheckExecutablePrivilege(10) == 1 && XexCheckExecutablePrivilege(9) == 0);
  u32 other = 1;
  CHECK(XexGetModuleHandle(0, &other) == 0 && other == h);
  other = 1;
  CHECK(XexGetModuleHandle("TITLE.xex", &other) == 0 && other == h);
  other = 1;
  CHECK((int32_t)XexGetModuleHandle("xbdm.xex", &other) < 0 && other == 0);
  u32 data = 0, size = 0;
  CHECK(XexGetModuleSection(h, "Strings", &data, &size) == 0 &&
        data == (u32)(uintptr_t)__xex_resource_Strings && size == sizeof __xex_resource_Strings);
  CHECK((int32_t)XexGetModuleSection(h, "Other", &data, &size) < 0);
  u32 address = 1;
  CHECK((int32_t)XexGetProcedureAddress(0, 1, &address) < 0 && address == 0);
  // The system modules: their exports by address, called through it.
  u32 xam = 0, krnl = 0;
  CHECK(XexGetModuleHandle("xam.xex", &xam) == 0 && xam && XexGetModuleHandle("XBOXKRNL.EXE", &krnl) == 0 &&
        krnl && krnl != xam && krnl != h);
  CHECK(XexGetProcedureAddress(xam, 75, &address) == 0 && address);
  CHECK(((u32(*)(u32))(uintptr_t)address)(1) == 0);  // NetDll_XNetGetEthernetLinkStatus: no link
  CHECK(XexGetProcedureAddress(krnl, 320, &address) == 0 && address);
  uint64_t time = 0x1da6b158f2a0f87ull;
  uint16_t f[8] = {0};
  ((u32(*)(const uint64_t*, uint16_t*))(uintptr_t)address)(&time, f);  // RtlTimeToTimeFields
  CHECK(f[0] == 2024 && f[1] == 2 && f[2] == 29 && f[7] == 4);
  CHECK(XexGetProcedureAddress(krnl, 344, &address) == 0 && address == __imp_XboxKrnlVersion);
  // Any export has an address, a handler or not (only a call without one is fatal).
  u32 first = 0;
  CHECK(XexGetProcedureAddress(xam, 1, &first) == 0 && XexGetProcedureAddress(xam, 2, &address) == 0 &&
        address == first + 4);
}

// ---- the console --------------------------------------------------------------
static void Console(void) {
  u32 v = 0;
  uint16_t required = 0;
  CHECK(ExGetXConfigSetting(3, 9, &v, 4, &required) == 0 && v == 1 && required == 4);  // English
  CHECK(ExGetXConfigSetting(2, 2, &v, 4, &required) == 0 && v == 0x00400100);  // NTSC-M
  required = 0;
  CHECK(ExGetXConfigSetting(3, 9, 0, 0, &required) == 0 && required == 4);
  CHECK(ExGetXConfigSetting(3, 9, &v, 2, &required) == 0xC0000023);
  // XGetAVPack and XGetVideoMode are the kernel's queries.
  CHECK(XGetAVPack() == HalGetCurrentAVPack());
  u32 mode[12], kernel_mode[12];
  XGetVideoMode(mode);
  VdQueryVideoMode(kernel_mode);
  CHECK(Memeq(mode, kernel_mode, sizeof mode));
  CHECK(mode[0] == 1280 && mode[1] == 720 && mode[3] == 1 && mode[5] == 0x42700000 && mode[6] == 1);
  CHECK(*At(__imp_VdGpuClockInMHz) == 500 && *At(__imp_VdGlobalDevice) == 0);
  // The calibration lock is a free critical section.
  void* lock = (void*)(uintptr_t)__imp_VdHSIOCalibrationLock;
  CHECK(At(__imp_VdHSIOCalibrationLock)[4] == 0xFFFFFFFF);
  RtlEnterCriticalSection(lock);
  CHECK(At(__imp_VdHSIOCalibrationLock)[4] == 0 && RtlTryEnterCriticalSection(lock) == 1);
  RtlLeaveCriticalSection(lock);
  RtlLeaveCriticalSection(lock);
  CHECK(At(__imp_VdHSIOCalibrationLock)[4] == 0xFFFFFFFF);
  CHECK(Memeq((const void*)(uintptr_t)__imp_ExLoadedCommandLine, "\"title.xex\"", 12));
  u32 registration[4] = {0x82001000, 0, 0, 0};
  CHECK(ExRegisterTitleTerminateNotification(registration, 1) == 0);
  CHECK(ExRegisterTitleTerminateNotification(registration, 0) == 0);
  CHECK(FscSetCacheElementCount(64) == 0);
}

// ---- the GPU -----------------------------------------------------------------
static volatile u32* Reg(u32 r) { return At(0x7FC80000 + 4 * r); }
static u32 Type0(u32 reg, u32 count) { return (count - 1) << 16 | reg; }
static u32 Type3(u32 opcode, u32 count) { return 0xC0000000 | (count - 1) << 16 | opcode << 8; }

// The graphics interrupt callback (start.S's interrupt_entry), as a DPC.
static volatile u32 interrupts[2], interrupt_user, interrupt_irql;
void GraphicsInterrupt(u32 source, u32 user) {
  if (source < 2) interrupts[source]++;
  interrupt_user = user;
  interrupt_irql = *(volatile uint8_t*)(uintptr_t)(pcr() + 0x18);
}

// Resolves, with the ring buffer `rb` at its write position `w`: EDRAM
// cleared through them and copied to tiled textures. Nothing is drawn,
// so the targets hold only what the clears put there.
static u32 ResolveAt(volatile u32* rb, u32 w, u32 verts, u32 control, u32 dest, u32 dest_info) {
  rb[w++] = Type0(0x4800, 2);  // vertex fetch 0: three points, 8 in 32
  rb[w++] = verts | 3;
  rb[w++] = 6 << 2 | 2;
  rb[w++] = Type0(0x2318, 4);  // RB_COPY_CONTROL, DEST_BASE, DEST_PITCH, DEST_INFO
  rb[w++] = control;
  rb[w++] = dest;
  rb[w++] = 64 | 64 << 16;
  rb[w++] = dest_info;
  rb[w++] = Type3(0x36, 1);  // DRAW_INDX_2: a rectangle list, 3 auto indices
  rb[w++] = 3 << 16 | 2 << 6 | 8;
  return w;
}

static u32 Resolves(volatile u32* rb, u32 w, u32 data, u32 pdata) {
  u32 mem = MmAllocatePhysicalMemoryEx(0, 0x14000, kPageReadWrite, 0, 0, 0x1000), pmem = MmGetPhysicalAddress(mem);
  u32 scratch = pmem, dest = pmem + 0x4000, depth = pmem + 0x8000, twice = pmem + 0x10000;
  for (u32 k = 0; k < 0x5000; k++) *At(mem + 4 * k) = 0xAAAAAAAA;
  // The rectangles' corners, pixel centres at integers (PA_SU_VTX_CNTL 0):
  // the whole 64x64 target, and 8x8 at (8, 16).
  volatile float* v = (volatile float*)(uintptr_t)(mem + 0xC000);
  const float corners[12] = {0, 0, 64, 0, 64, 64, 8, 16, 16, 16, 16, 24};
  for (u32 k = 0; k < 12; k++) v[k] = corners[k];
  u32 whole = pmem + 0xC000, block = whole + 24;
  // A 64-pixel pitch without MSAA; colour target 0 at tile 0 (8888),
  // depth at tile 0x100; scissors open; copy mode.
  rb[w++] = Type0(0x2000, 3);
  rb[w++] = 64;
  rb[w++] = 0;
  rb[w++] = 0x100;
  rb[w++] = Type0(0x200E, 2);
  rb[w++] = 0;
  rb[w++] = 8192 | 8192 << 16;
  rb[w++] = Type0(0x2080, 3);
  rb[w++] = 0;
  rb[w++] = 0x80000000;
  rb[w++] = 64 | 64 << 16;
  rb[w++] = Type0(0x2205, 1);
  rb[w++] = 0;
  rb[w++] = Type0(0x2302, 1);
  rb[w++] = 0;
  rb[w++] = Type0(0x2208, 1);
  rb[w++] = 6;
  rb[w++] = Type0(0x231D, 3);  // RB_DEPTH_CLEAR, RB_COLOR_CLEAR, _LO
  rb[w++] = 0xABCDEF01;
  rb[w++] = 0x11223344;
  rb[w++] = 0;
  // The target cleared to A, then the block to B (each copy goes to the
  // scratch texture first, before its clear).
  w = ResolveAt(rb, w, whole, 0x100, scratch, 6 << 7);
  rb[w++] = Type0(0x231E, 1);
  rb[w++] = 0x55667788;
  w = ResolveAt(rb, w, block, 0x100, scratch, 6 << 7);
  // The target, red and blue swapped, bytes in words swapped (so this
  // big-endian CPU reads the swapped value).
  w = ResolveAt(rb, w, whole, 0, dest, 1u << 24 | 6 << 7 | 2);
  // The target to another texture, cleared to C, and then the block (now
  // C) to it too: the second resolve's 32x32 texels around the block
  // keep what the first wrote.
  rb[w++] = Type0(0x231E, 1);
  rb[w++] = 0x99AABBCC;
  w = ResolveAt(rb, w, whole, 0x100, twice, 6 << 7);
  w = ResolveAt(rb, w, block, 0, twice, 6 << 7);
  // The depth target cleared, then copied.
  w = ResolveAt(rb, w, whole, 0x204, scratch, 6 << 7);
  w = ResolveAt(rb, w, whole, 4, depth, 6 << 7 | 2);
  // A fence, then a wait on SCRATCH_REG1, a register, which holds the
  // GPU until the texels are checked: they are in memory by the fence,
  // not only by the end of the commands.
  rb[w++] = Type3(0x58, 3);
  rb[w++] = 0;
  rb[w++] = (pdata + 0x68) | 2;
  rb[w++] = 5;
  rb[w++] = Type3(0x3C, 5);
  rb[w++] = 3;
  rb[w++] = 0x579;
  rb[w++] = 1;
  rb[w++] = 0xFFFFFFFF;
  rb[w++] = 0x100;
  *Reg(0x1C5) = w;
  CHECK(WaitFor(At(data + 0x68), 5));
  // A is 11223344 (8888 as the EDRAM holds it, red in the low byte), B
  // 55667788; swapped: 11443322 and 55887766. Texel (8, 16) of a 64-texel
  // pitch of 4-byte texels is at 0x840 (bank 1, pipe 1), (9, 16) at
  // 0x844, (8, 17) at 0x850, (15, 23) at 0xB7C; (16, 16) is outside the
  // block at 0x880.
  u32 a = 0, b = 0;
  for (u32 k = 0; k < 0x1000; k++) {
    u32 t = *At(mem + 0x4000 + 4 * k);
    a += t == 0x11443322;
    b += t == 0x55887766;
  }
  CHECK(a == 4096 - 64 && b == 64);
  CHECK(*At(mem + 0x4840) == 0x55887766 && *At(mem + 0x4844) == 0x55887766 && *At(mem + 0x4850) == 0x55887766 &&
        *At(mem + 0x4B7C) == 0x55887766 && *At(mem + 0x4000) == 0x11443322 && *At(mem + 0x4880) == 0x11443322);
  u32 d = 0;
  for (u32 k = 0; k < 0x1000; k++) d += *At(mem + 0x8000 + 4 * k) == 0xABCDEF01;
  CHECK(d == 4096);
  // In this one, words as the EDRAM holds them: A, and C in the block.
  u32 c = 0;
  a = 0;
  for (u32 k = 0; k < 0x1000; k++) {
    u32 t = *At(mem + 0x10000 + 4 * k);
    a += t == 0x44332211;
    c += t == 0xCCBBAA99;
  }
  CHECK(a == 4096 - 64 && c == 64);
  *Reg(0x579) = 1;

  // The resolved texture as the front buffer, twice: tiled, a 64-texel
  // pitch, 64x64, its words byte-swapped (endian 2) and the swizzle ZYXW,
  // so A shows as red 44, green 33, blue 22 and B as 88 77 66. A swap
  // waits until the one before it is shown, so after the fence the first
  // of the two has been (xbox360_runtime_frame checks the screenshot).
  u32 tex[6] = {1u << 31 | 2 << 22, (mem + 0x4000) | 2 << 6 | 6, 63 | 63 << 13, (2 | 1 << 3 | 0 << 6 | 3 << 9) << 1,
                0, 0},
      front = mem + 0x4000, format = 6, color_space = 0, width = 64, height = 64;
  for (u32 n = 0; n < 2; n++) {
    VdSwap10(&rb[w], tex, 0, 0, 0, &front, &format, &color_space, &width, &height);
    w += 64;
  }
  rb[w++] = Type3(0x58, 3);
  rb[w++] = 0;
  rb[w++] = (pdata + 0x70) | 2;
  rb[w++] = 7;
  *Reg(0x1C5) = w;
  CHECK(WaitFor(At(data + 0x70), 7));

  // A converting copy: a 2x MSAA 7e3 target (format 3, at tile 0x200)
  // cleared to red 16 (0x380), green 0.375 (0x0C0), blue 2^-9 (0x001, a
  // denormal: 1/128 of 2^-2), alpha 1 (3), its two samples averaged into
  // half floats (16_16_16_16_FLOAT, 64-bit texels): 4C00 3600 1800 3C00.
  u32 wide = MmAllocatePhysicalMemoryEx(0, 0x8000, kPageReadWrite, 0, 0, 0x1000), pwide = MmGetPhysicalAddress(wide);
  for (u32 k = 0; k < 0x2000; k++) *At(wide + 4 * k) = 0xAAAAAAAA;
  rb[w++] = Type0(0x2000, 2);
  rb[w++] = 64 | 1 << 16;
  rb[w++] = 3 << 16 | 0x200;
  rb[w++] = Type0(0x231E, 1);
  rb[w++] = 0xC0130380;
  w = ResolveAt(rb, w, whole, 0x100, pwide, 32 << 7 | 2);
  w = ResolveAt(rb, w, whole, 6 << 4, pwide, 32 << 7 | 2);
  rb[w++] = Type3(0x58, 3);
  rb[w++] = 0;
  rb[w++] = (pdata + 0x6C) | 2;
  rb[w++] = 6;
  *Reg(0x1C5) = w;
  CHECK(WaitFor(At(data + 0x6C), 6));
  u32 h = 0;
  for (u32 k = 0; k < 0x1000; k++) h += *At(wide + 8 * k) == 0x36004C00 && *At(wide + 8 * k + 4) == 0x3C001800;
  CHECK(h == 4096);
  return w;
}

// Shader microcode, built from its fields (docs/xenos.md, "Shaders").
// Control flow comes first, two 48-bit instructions in three words: an
// exec_end (opcode 2) of `n` instructions from instruction `at` (three
// words each), those whose bit is set in `fetches` fetches, then a nop.
static void UcodeExec(u32* w, u32 at, u32 n, u32 fetches) {
  u32 sequence = 0;
  for (u32 k = 0; k < n; k++) sequence |= (fetches >> k & 1) << (2 * k);
  w[0] = at | n << 12 | sequence << 16;
  w[1] = 2u << 12;
  w[2] = 0;
}
// A vertex fetch of vertex fetch constant `constant` by the vertex index
// (r0.x) into register `dst`, components as fetched (swizzle xyzw):
// `format`, `stride` words a vertex, `offset` words into it.
static void UcodeVfetch(u32* w, u32 dst, u32 constant, u32 format, u32 stride, u32 offset) {
  w[0] = dst << 12 | constant / 3 << 20 | constant % 3 << 25;
  w[1] = (0 | 1 << 3 | 2 << 6 | 3 << 9) | format << 16;
  w[2] = stride | offset << 8;
}
// An ALU instruction that exports max(a, a), a being temporary register
// or float constant `src`, all four components, to export register `dest`
// (vector opcode 2, no scalar operation: opcode 50 with no mask).
static void UcodeExport(u32* w, u32 dest, u32 src, int temp) {
  w[0] = dest | 1 << 15 | 0xFu << 16 | 50u << 26;
  w[1] = 0;
  w[2] = src << 16 | src << 8 | 2u << 24 | (temp ? 3u << 30 : 0);
}

// Byte offset of texel (x, y) of a tiled texture 64 texels wide, texels
// of 1 << log2 bytes (edram.cpp's Tiled2D).
static u32 Tiled64(u32 x, u32 y, u32 log2) {
  u32 outer = ((y >> 5) * 2 + (x >> 5)) << 6, inner = ((y >> 1) & 7) << 3 | (x & 7);
  u32 oi = (outer | inner) << log2, bank = (y >> 4) & 1, pipe = ((x >> 3) & 3) ^ (((y >> 3) & 1) << 1);
  return (oi & 15) | (y & 1) << 4 | ((oi >> 4) & 1) << 5 | pipe << 6 | ((oi >> 5) & 7) << 8 | bank << 11 |
         (oi >> 8) << 12;
}

static u32 FloatBits(float f) {
  union {
    float f;
    u32 u;
  } v = {f};
  return v.u;
}

// What the draws' tests write: an indirect buffer the ring runs (then a
// fence), and vertices.
static struct {
  volatile u32* rb;
  u32 w, data, pdata, fence;
  volatile u32* ib;
  u32 n, pib;
  u32 pool, ppool, used;  // vertices
  u32 corners, small;     // the resolves' rectangles: 64x64, 16x16
  u32 ps;                 // the pixel shaders' microcode, 0x40 bytes each
  u32 vs;                 // the vertex shaders' microcode, 0x80 bytes each
} gq;
static void Put(u32 v) { gq.ib[gq.n++] = v; }
static void Set(u32 reg, u32 v) {
  Put(Type0(reg, 1));
  Put(v);
}
static void SetF(u32 reg, float a, float b, float c, float d) {
  Put(Type0(reg, 4));
  Put(FloatBits(a));
  Put(FloatBits(b));
  Put(FloatBits(c));
  Put(FloatBits(d));
}
// Runs the buffer and waits for its fence (up to ten seconds: a host
// GPU in software is slow).
static void Kick(void) {
  volatile u32* rb = gq.rb;
  rb[gq.w++] = Type3(0x3F, 2);
  rb[gq.w++] = gq.pib;
  rb[gq.w++] = gq.n;
  rb[gq.w++] = Type3(0x58, 3);
  rb[gq.w++] = 0;
  rb[gq.w++] = (gq.pdata + 0x78) | 2;
  rb[gq.w++] = ++gq.fence;
  *Reg(0x1C5) = gq.w;
  int done = 0;
  for (int k = 0; k < 10 && !done; k++) done = WaitFor(At(gq.data + 0x78), gq.fence);
  CHECK(done);
  gq.n = 0;
}
// Vertices x, y (pairs), z and a colour, seven words each: their physical
// address.
static u32 Verts(const float* xy, u32 n, float z, const float* c) {
  u32 pa = gq.ppool + gq.used;
  for (u32 v = 0; v < n; v++) {
    const float f[7] = {xy[2 * v], xy[2 * v + 1], z, c[0], c[1], c[2], c[3]};
    for (u32 k = 0; k < 7; k++) *At(gq.pool + gq.used + 4 * k) = FloatBits(f[k]);
    gq.used += 28;
  }
  return pa;
}
static void Fetch0(u32 pa, u32 vertices) {
  Put(Type0(0x4800, 2));
  Put(pa | 3);
  Put(7 * vertices << 2 | 2);
}
// A triangle, or a rectangle of corners (x0, y0) and (x1, y1), counted
// indices.
static void Tri(const float xy[6], float z, const float* c) {
  Fetch0(Verts(xy, 3, z, c), 3);
  Put(Type3(0x36, 1));
  Put(3 << 16 | 2 << 6 | 4);
}
static void Rect(float x0, float y0, float x1, float y1, float z, const float* c) {
  const float xy[6] = {x0, y0, x1, y0, x0, y1};
  Fetch0(Verts(xy, 3, z, c), 3);
  Put(Type3(0x36, 1));
  Put(3 << 16 | 2 << 6 | 8);
}
// A resolve of the 64x64 (or 16x16) target to a texture 64 texels wide,
// in copy mode, back to drawing colour and depth after it.
static void Resolve(u32 control, u32 dest, u32 dest_info, int small) {
  Set(0x2208, 6);
  gq.n = ResolveAt(gq.ib, gq.n, small ? gq.small : gq.corners, control, dest, dest_info);
  Set(0x2208, 4);
}
// The pixel shader that exports r0 (0), c0 (1), r0 and c0 to colours 0
// and 1 (2), r0 and depth c0.x (3), r1 (4).
static void PixelShader(u32 k) {
  Put(Type3(0x27, 2));  // IM_LOAD
  Put((gq.ps + 0x40 * k) | 1);
  Put(k == 2 || k == 3 ? 9 : 6);
}
// The vertex shader that exports the position and the colour to
// interpolator 0 (0), and the colour as the point size, edge flag and
// kill too (1).
static void VertexShader(u32 k) {
  Put(Type3(0x27, 2));
  Put(gq.vs + 0x80 * k);
  Put(k ? 18 : 15);
}

static const float kWhite[4] = {1, 1, 1, 1}, kBlack[4] = {0, 0, 0, 0}, kRed[4] = {1, 0, 0, 1},
                   kGreen[4] = {0, 1, 0, 1}, kBlue[4] = {0, 0, 1, 1};

// Each colour target format drawn as float constant 0 and its EDRAM words
// resolved as they are (the target read as 32_FLOAT or 32_32_FLOAT).
static const struct {
  u32 info;  // format, exponent bias
  float c[4];
  u32 w0, w1;
} kFormats[] = {
    {0, {1, 0.2f, 0, 1}, 0xFF0033FF, 0},
    {1, {1, 0.25f, 0, 1}, 0xFF007FFF, 0},  // gamma: 0.25 to 127/255
    {2, {1, 0.25f, 0, 1}, 0xC00403FF, 0},
    {10, {1, 0.25f, 0, 1}, 0xC00403FF, 0},
    // 7e3: 0.125 a denormal (0x040), 40 above the range (31.875, 0x3FF),
    // 1 (0x180).
    {3, {0.125f, 40, 1, 1}, 0xD80FFC40, 0},
    {12, {0.125f, 40, 1, 1}, 0xD80FFC40, 0},
    {4, {4, -8, 0, 0}, 0xE0001000, 0},  // fixed point: 4096 a 32767/32 per unit
    {5, {4, -8, 1, -1}, 0xE0001000, 0xFC000400},
    {6, {1.5f, -2, 0, 0}, 0xC0003E00, 0},
    {6 | 1 << 4, {1.5f, -2, 0, 0}, 0xC4004200, 0},  // exponent bias 1: 3, -4
    {7, {1.5f, -2, 0.25f, 1}, 0xC0003E00, 0x3C003400},
    {14, {1.5f, 0, 0, 0}, 0x3FC00000, 0},
    {15, {1.5f, -2, 0, 0}, 0x3FC00000, 0xC0000000},
};

// A draw of `n` vertices at `pa`, counted indices.
static void DrawAuto(u32 prim, u32 pa, u32 n) {
  Fetch0(pa, n);
  Put(Type3(0x36, 1));
  Put(n << 16 | 2 << 6 | prim);
}
// A draw of `count` 16-bit indices at `indices` (as this big-endian CPU
// stores them: bytes swapped in halfwords) into `n` vertices at `pa`.
static void DrawIndexed(u32 prim, u32 pa, u32 n, u32 indices, u32 count) {
  Fetch0(pa, n);
  Put(Type3(0x22, 4));
  Put(0);
  Put(count << 16 | 0 << 6 | prim);
  Put(indices);
  Put(count | 1u << 30);
}
// 16-bit indices in the vertex pool: their physical address.
static u32 Indices16(const uint16_t* v, u32 n) {
  u32 pa = gq.ppool + gq.used;
  volatile uint16_t* p = (volatile uint16_t*)(uintptr_t)(gq.pool + gq.used);
  for (u32 k = 0; k < n; k++) p[k] = v[k];
  gq.used = (gq.used + 2 * n + 3) & ~3u;
  return pa;
}
// One vertex in a colour.
static u32 Vert(float x, float y, float r, float g, float b, float a) {
  const float xy[2] = {x, y}, c[4] = {r, g, b, a};
  return Verts(xy, 1, 0, c);
}

// The other primitive types, points and what the pixel shader gets from
// the rasteriser: 8_8_8_8 targets E at 0x040 and E2 at 0x044, a
// 16_16_16_16_FLOAT target F at 0x050 and, 4x MSAA, F4 at 0x060.
static void Primitives(void) {
  u32 mem = MmAllocatePhysicalMemoryEx(0, 0x40000, kPageReadWrite, 0, 0, 0x1000), pmem = MmGetPhysicalAddress(mem);
  for (u32 k = 0; k < 0x40000; k += 4) *At(mem + k) = 0xAAAAAAAA;
  u32 dest_e = pmem, dest_e2 = pmem + 0x4000, dest_f = pmem + 0x8000, dest_f4 = pmem + 0x10000,
      dest_c = pmem + 0x18000, dest_d = pmem + 0x1C000;
  Set(0x2000, 64);
  Set(0x231E, 0);
  Set(0x2001, 0x040);
  Resolve(0x100, dest_e, 6 << 7 | 2, 0);
  Set(0x2001, 0x044);
  Resolve(0x100, dest_e2, 6 << 7 | 2, 0);
  Set(0x2001, 0x040);
  PixelShader(0);

  // A fan, its triangles (v1 v2 v0), (v2 v3 v0): the square (0-16), its
  // lower left (2, 13) too, which a strip of these leaves out.
  u32 fan = Vert(0, 0, 1, 1, 1, 1);
  Vert(16, 0, 1, 1, 1, 1);
  Vert(16, 16, 1, 1, 1, 1);
  Vert(0, 16, 1, 1, 1, 1);
  DrawAuto(5, fan, 4);
  // A strip of 16-bit indices with a reset index (0xFFFF), back faces
  // culled, front counter-clockwise: (16 0) (16 16) (32 0) (32 16), its
  // odd triangle wound as the even one; then (36 0) (36 16) (48 0) alone,
  // nothing across the reset at (34, 8).
  u32 strip = Vert(16, 0, 1, 1, 1, 1);
  Vert(16, 16, 1, 1, 1, 1);
  Vert(32, 0, 1, 1, 1, 1);
  Vert(32, 16, 1, 1, 1, 1);
  Vert(36, 0, 1, 1, 1, 1);
  Vert(36, 16, 1, 1, 1, 1);
  Vert(48, 0, 1, 1, 1, 1);
  const uint16_t strip_indices[8] = {0, 1, 2, 3, 0xFFFF, 4, 5, 6};
  u32 pstrip = Indices16(strip_indices, 8);
  Set(0x2103, 0xFFFF);           // VGT_MULTI_PRIM_IB_RESET_INDX
  Set(0x2205, 1 << 21 | 2);      // multi_prim_ib_ena, back faces culled
  DrawIndexed(6, strip, 7, pstrip, 8);
  Set(0x2205, 0);
  // A quad (0 16) red (16 16) (16 32) blue (0 32), split along (0 16)-(16
  // 32): above it no blue, below it no red.
  u32 quad = Vert(0, 16, 0, 0, 0, 1);
  Vert(16, 16, 1, 0, 0, 1);
  Vert(16, 32, 0, 0, 0, 1);
  Vert(0, 32, 0, 0, 1, 1);
  DrawAuto(13, quad, 4);
  // Points of PA_SU_POINT_SIZE's size, 4 (half 2: 32 in 12.4), at (8, 50):
  // pixels 6-9, 48-51.
  Set(0x2280, 32 << 16 | 32);
  Set(0x2281, 8 | 32 << 16);  // PA_SU_POINT_MINMAX: half sizes 0.5, 2
  DrawAuto(1, Vert(8, 50, 1, 1, 1, 1), 1);
  // The vertex shader's size (its colour's red), clamped: 6 to 4 at
  // (20, 50), pixels 18-21; 2 at (30, 50), pixels 29-30.
  VertexShader(1);
  u32 sized = Vert(20, 50, 6, 1, 0, 1);
  Vert(30, 50, 2, 1, 0, 1);
  DrawAuto(1, sized, 2);
  VertexShader(0);
  Resolve(0, dest_e, 6 << 7 | 2, 0);

  // E2: lines, a list (16 2)-(40 2), a strip (16 6) (40 6) (40 14), a loop
  // (44 2) (56 2) (56 12) (44 12), back along x 44.
  Set(0x2001, 0x044);
  u32 lines = Vert(16, 2, 1, 1, 1, 1);
  Vert(40, 2, 1, 1, 1, 1);
  DrawAuto(2, lines, 2);
  u32 line_strip = Vert(16, 6, 1, 1, 1, 1);
  Vert(40, 6, 1, 1, 1, 1);
  Vert(40, 14, 1, 1, 1, 1);
  DrawAuto(3, line_strip, 3);
  u32 loop = Vert(44, 2, 1, 1, 1, 1);
  Vert(56, 2, 1, 1, 1, 1);
  Vert(56, 12, 1, 1, 1, 1);
  Vert(44, 12, 1, 1, 1, 1);
  DrawAuto(12, loop, 4);
  // Indices 0 1 5 with VGT_MAX_VTX_INDX 2: the triangle (48 16) (64 16)
  // (64 32), not (48 32), vertex 5.
  u32 clamped = Vert(48, 16, 1, 1, 1, 1);
  Vert(64, 16, 1, 1, 1, 1);
  Vert(64, 32, 1, 1, 1, 1);
  Vert(0, 0, 1, 1, 1, 1);
  Vert(0, 0, 1, 1, 1, 1);
  Vert(48, 32, 1, 1, 1, 1);
  const uint16_t clamped_indices[3] = {0, 1, 5};
  u32 pclamped = Indices16(clamped_indices, 3);
  Set(0x2100, 2);
  DrawIndexed(4, clamped, 6, pclamped, 3);
  Set(0x2100, 0xFFFFFF);
  // Immediate 16-bit indices 2 0 1 (in the packet, two a word, the low
  // half first): the triangle (0 16) (16 16) (0 32).
  u32 immediate = Vert(16, 16, 1, 1, 1, 1);
  Vert(0, 32, 1, 1, 1, 1);
  Vert(0, 16, 1, 1, 1, 1);
  Fetch0(immediate, 3);
  Put(Type3(0x22, 4));  // DRAW_INDX: query word, initiator, indices
  Put(0);
  Put(3 << 16 | 1 << 6 | 4);
  Put(2 | 0 << 16);
  Put(1);
  // Vertex kill (the colour's blue): with all of a triangle's vertices
  // needed (PA_CL_CLIP_CNTL bit 21 clear), one killed keeps (16 40)-(32
  // 56), three drop (32 40)-(48 56); with any (bit 21 set), one drops (48
  // 40)-(64 56).
  VertexShader(1);
  u32 kill = Vert(16, 40, 1, 1, 0, 1);
  Vert(32, 40, 1, 1, 0, 1);
  Vert(32, 56, 1, 1, 1, 1);
  Vert(32, 40, 1, 1, 1, 1);
  Vert(48, 40, 1, 1, 1, 1);
  Vert(48, 56, 1, 1, 1, 1);
  DrawAuto(4, kill, 6);
  u32 kill_or = Vert(48, 40, 1, 1, 0, 1);
  Vert(64, 40, 1, 1, 0, 1);
  Vert(64, 56, 1, 1, 1, 1);
  Set(0x2204, 1 << 16 | 1 << 21);
  DrawAuto(4, kill_or, 3);
  Set(0x2204, 1 << 16);
  VertexShader(0);
  Resolve(0, dest_e2, 6 << 7 | 2, 0);

  // The pixel parameters (r1, SQ_CONTEXT_MISC bits 8-15) in F: front (4, 4)
  // and back (28, 4) faces, X negative for the back one; a point sprite
  // of 8 at (40, 40): at (37, 37) Y negative, the sprite coordinates 1/8.
  // Then F4 (4x): a sprite of 8 at (20, 20), sample 0 of (17, 17) 3/32
  // into it.
  Set(0x2001, 0x050 | 7 << 16);
  Set(0x231E, 0);
  Set(0x231F, 0);
  Resolve(0x100, dest_f, 32 << 7 | 2, 0);
  Set(0x2180, 1 << 18);  // SQ_PROGRAM_CNTL: param_gen
  Set(0x2181, 1 << 8);   // SQ_CONTEXT_MISC: param_gen_pos 1
  PixelShader(4);
  const float front[6] = {0, 0, 0, 16, 16, 0}, back[6] = {16, 0, 32, 0, 32, 16};
  Tri(front, 0, kWhite);
  Tri(back, 0, kWhite);
  Set(0x2280, 64 << 16 | 64);
  Set(0x2281, 0xFFFF0000);
  DrawAuto(1, Vert(40, 40, 1, 1, 1, 1), 1);
  Resolve(0, dest_f, 32 << 7 | 2, 0);
  Set(0x2000, 64 | 2 << 16);
  Set(0x2001, 0x060 | 7 << 16);
  Resolve(0x100, dest_f4, 32 << 7 | 2, 0);
  DrawAuto(1, Vert(20, 20, 1, 1, 1, 1), 1);
  Resolve(0, dest_f4, 32 << 7 | 2, 0);
  Set(0x2000, 64);
  Set(0x2180, 0);
  Set(0x2181, 0);
  PixelShader(0);

  // A scissor of 8192 (window and screen) with colour at 0x0C0 and depth
  // right after it at 0x0C4: each holds the surface's rows up to the
  // other. Depth alone first (depth only), so the depth target is stored
  // before the colour one; then both, depth 0.5 at (8-16, 8-16): the
  // colour target's rows must not reach the depth's tiles.
  Put(Type0(0x2080, 3));
  Put(0);
  Put(0x80000000);
  Put(8192 | 8192 << 16);
  Set(0x2001, 0x0C0);
  Set(0x2002, 0x0C4);
  Set(0x231D, 0xFFFFFF00);
  Resolve(0x100, dest_c, 6 << 7 | 2, 0);
  Resolve(0x200 | 4, dest_d, 6 << 7 | 2, 0);
  Set(0x2200, 2 | 4 | 7 << 4);
  Set(0x2208, 5);
  Rect(40, 40, 48, 48, 1, kGreen);
  Set(0x2208, 4);
  Rect(8, 8, 16, 16, 0.5f, kGreen);
  Set(0x2200, 0);
  Resolve(0, dest_c, 6 << 7 | 2, 0);
  Resolve(4, dest_d, 6 << 7 | 2, 0);
  Put(Type0(0x2080, 3));
  Put(0);
  Put(0x80000000);
  Put(64 | 64 << 16);
  Set(0x2001, 0x600);
  Set(0x2002, 0x500);
  Kick();

#define E(x, y) At(mem)[Tiled64(x, y, 2) / 4]
#define E2(x, y) At(mem + 0x4000)[Tiled64(x, y, 2) / 4]
  CHECK(E(2, 13) == 0xFFFFFFFF && E(13, 2) == 0xFFFFFFFF);
  CHECK(E(18, 4) == 0xFFFFFFFF && E(29, 12) == 0xFFFFFFFF && E(38, 4) == 0xFFFFFFFF && E(34, 8) == 0);
  CHECK((E(13, 19) & 0xFF0000) == 0 && (E(13, 19) & 0xFF) != 0 && (E(3, 29) & 0xFF) == 0 &&
        (E(3, 29) & 0xFF0000) != 0);
  CHECK(E(9, 51) == 0xFFFFFFFF && E(6, 48) == 0xFFFFFFFF && E(10, 50) == 0 && E(5, 50) == 0);
  CHECK(E(21, 51) == 0xFF00FFFF && E(18, 48) == 0xFF00FFFF && E(22, 50) == 0 && E(17, 50) == 0);
  CHECK(E(29, 50) == 0xFF00FFFF && E(30, 50) == 0xFF00FFFF && E(28, 50) == 0 && E(31, 50) == 0);
  CHECK(E2(28, 2) == 0xFFFFFFFF && E2(28, 6) == 0xFFFFFFFF && E2(40, 10) == 0xFFFFFFFF);
  CHECK(E2(44, 7) == 0xFFFFFFFF && E2(56, 7) == 0xFFFFFFFF && E2(50, 2) == 0xFFFFFFFF);
  CHECK(E2(60, 20) == 0xFFFFFFFF && E2(50, 28) == 0);
  CHECK(E2(4, 20) == 0xFFFFFFFF);
  CHECK(E2(20, 42) != 0 && E2(44, 44) == 0 && E2(60, 42) == 0);
  volatile u32* f = At(mem + 0x8000);
  volatile u32* f4 = At(mem + 0x10000);
#define F(t, x, y, k) t[Tiled64(x, y, 3) / 4 + (k)]
  CHECK(F(f, 4, 4, 0) == 0x44004400 && F(f, 4, 4, 1) == 0);
  CHECK(F(f, 28, 4, 0) == 0x4400CF00 && F(f, 28, 4, 1) == 0);
  CHECK(F(f, 37, 37, 0) == 0xD0A050A0 && F(f, 37, 37, 1) == 0x30003000);
  CHECK(F(f4, 17, 17, 0) == 0xCC404C40 && F(f4, 17, 17, 1) == 0x2E002E00);
  CHECK(At(mem + 0x18000)[Tiled64(10, 10, 2) / 4] == 0xFF00FF00 &&
        At(mem + 0x1C000)[Tiled64(10, 10, 2) / 4] == 0x80000000);
#undef E
#undef E2
#undef F
}

// ---- textures -------------------------------------------------------------
static u32 Bswap32(u32 v) { return v >> 24 | (v >> 8 & 0xFF00) | (v << 8 & 0xFF0000) | v << 24; }
// What the CPU stores for the GPU to read `v` with endian swap `e`: the
// GPU reads words little-endian, then swaps.
static u32 GuestWord(u32 v, u32 e) {
  if (e == 1) v = (v << 8 & 0xFF00FF00) | (v >> 8 & 0x00FF00FF);
  if (e == 2) v = Bswap32(v);
  if (e == 3) v = v >> 16 | v << 16;
  return Bswap32(v);
}
// Byte offset of block (x, y) of a tiled surface `pitch` blocks wide.
static u32 TiledAt(u32 x, u32 y, u32 pitch, u32 log2) {
  u32 outer = ((y >> 5) * (pitch >> 5) + (x >> 5)) << 6, inner = ((y >> 1) & 7) << 3 | (x & 7);
  u32 oi = (outer | inner) << log2, bank = (y >> 4) & 1, pipe = ((x >> 3) & 3) ^ (((y >> 3) & 1) << 1);
  return (oi & 15) | (y & 1) << 4 | ((oi >> 4) & 1) << 5 | pipe << 6 | ((oi >> 5) & 7) << 8 | bank << 11 |
         (oi >> 8) << 12;
}

// A texture fetch constant (xe_gpu_texture_fetch_t) n, from its fields.
struct Tex {
  u32 format, pa, w, h, d;  // d: a 3D texture's depth, a stacked one's layers
  u32 dim, stacked, tiled, pitch, endian;
  u32 signs, clamp, swizzle, exp, filter, mip_filter, vol_linear;
  u32 mip_max, packed, mip_pa;
};
static struct Tex MakeTex(u32 format, u32 pa, u32 w, u32 h, u32 d, u32 dim, u32 stacked, u32 tiled, u32 pitch,
                          u32 endian, u32 signs, u32 clamp, u32 swizzle) {
  struct Tex t = {0};
  t.format = format, t.pa = pa, t.w = w, t.h = h, t.d = d, t.dim = dim, t.stacked = stacked, t.tiled = tiled;
  t.pitch = pitch, t.endian = endian, t.signs = signs, t.clamp = clamp, t.swizzle = swizzle;
  return t;
}
static void TexConst(u32 n, const struct Tex* t) {
  u32 w2 = t->dim == 2 ? (t->w - 1) | (t->h - 1) << 11 | (t->d - 1) << 22
                       : (t->w - 1) | (t->h - 1) << 13 | (t->stacked ? t->d - 1 : t->dim == 3 ? 5 : 0) << 26;
  Put(Type0(0x4800 + 6 * n, 6));
  Put(2 | t->signs << 2 | t->clamp * 0x49 << 10 | (t->pitch / 32) << 22 | t->tiled << 31);
  Put(t->format | t->endian << 6 | t->stacked << 10 | (t->pa >> 12) << 12);
  Put(w2);
  Put(t->swizzle << 1 | (t->exp & 63) << 13 | t->filter * 0x5 << 19 | t->mip_filter << 23);
  Put(t->vol_linear * 3 | t->mip_max << 6);
  Put(t->dim << 9 | t->packed << 11 | (t->mip_pa >> 12) << 12);
}
#define XYZW (0 | 1 << 3 | 2 << 6 | 3 << 9)

// An ALU instruction: vector operation `vop` of a, b, c (a register when
// the operand's temp bit is set, else a float constant; a swizzle each,
// the component each result component takes) to register or export
// `dest` (mask), no scalar operation unless `sop` < 50 with its own mask
// and register.
struct AluOp {
  u32 reg, temp, swizzle;  // swizzle: 2 bits a component, the source component
};
static u32 Swz(u32 x, u32 y, u32 z, u32 w) {
  const u32 c[4] = {x, y, z, w};
  u32 s = 0;
  for (u32 k = 0; k < 4; k++) s |= ((c[k] - k) & 3) << (2 * k);
  return s;
}
static void UcodeAlu(u32* w, u32 vop, u32 dest, u32 mask, int exp, struct AluOp a, struct AluOp b, struct AluOp c,
                     u32 sop, u32 sdest, u32 smask) {
  w[0] = dest | sdest << 8 | (u32)exp << 15 | mask << 16 | smask << 20 | sop << 26;
  w[1] = c.swizzle | b.swizzle << 8 | a.swizzle << 16;
  w[2] = c.reg | b.reg << 8 | a.reg << 16 | vop << 24 | c.temp << 29 | b.temp << 30 | a.temp << 31;
}
// A texture fetch: operation `op` (1 sample, 24 setTexLOD) of texture
// constant n at register src (components xyz as swz says) into register
// dst; `y` and `z` the instruction's filters and flags beyond these.
static void UcodeTfetch(u32* w, u32 op, u32 dst, u32 src, u32 swz, u32 n, u32 dim, u32 y, u32 z) {
  w[0] = op | src << 5 | dst << 12 | n << 20 | swz << 26;
  w[1] = XYZW | 0xFu << 24 | y;
  w[2] = dim << 14 | z;
}
// The filters an instruction leaves to the fetch constant.
#define TF_CONST (3 << 12 | 3 << 14 | 3 << 16 | 7 << 18)

// A pixel shader sampling texture constant n of dimension `dim` at
// interpolator 0's xyz, LOD from float constant 0's x where `y` asks for
// the register LOD, to colour 0: its microcode in the pool.
static u32 PsTex(u32 n, u32 dim, u32 y, u32 z) {
  u32 w[15];
  const struct AluOp c0 = {0, 0, 0}, r2 = {2, 1, 0};
  UcodeExec(w, 1, 4, 6);
  UcodeAlu(w + 3, 2, 1, 0xF, 0, c0, c0, c0, 50, 0, 0);  // r1 = c0
  UcodeTfetch(w + 6, 24, 0, 1, 0, 0, 0, TF_CONST, 0);   // setTexLOD r1.x
  UcodeTfetch(w + 9, 1, 2, 0, 0 | 1 << 2 | 2 << 4, n, dim, y, z);
  UcodeAlu(w + 12, 2, 0, 0xF, 1, r2, r2, r2, 50, 0, 0);  // colour 0 = r2
  u32 pa = gq.ppool + gq.used;
  for (u32 k = 0; k < 15; k++) *At(gq.pool + gq.used + 4 * k) = w[k];
  gq.used += 64;
  return pa;
}
static void LoadPs(u32 pa, u32 words) {
  Put(Type3(0x27, 2));
  Put(pa | 1);
  Put(words);
}
// One pixel (x, y) of the target drawn with interpolator 0 (u, v, w, 0).
static void Pixel(u32 x, u32 y, float u, float v, float w) {
  const float c[4] = {u, v, w, 0};
  Rect((float)x, (float)y, (float)(x + 1), (float)(y + 1), 0, c);
}

// Textures sampled into an 8_8_8_8 target at 0x200 (texels as their
// words) and a 32_32_FLOAT one at 0x210, resolved and read.
static void Textures(void) {
  u32 mem = MmAllocatePhysicalMemoryEx(0, 0x100000, kPageReadWrite, 0, 0, 0x1000), pmem = MmGetPhysicalAddress(mem);
  for (u32 k = 0; k < 0x100000; k += 4) *At(mem + k) = 0;
  u32 dest = pmem + 0xF0000, dest_f = pmem + 0xF4000;  // 64x64 and 16x16 (8-byte) resolves
  Set(0x2000, 64);
  Set(0x2001, 0x200);
  Set(0x231E, 0x11111111);
  Resolve(0x100, dest, 6 << 7 | 2, 0);
  u32 n_pixel = 0;
#define NEXT (n_pixel++)
#define PX(k) ((k) % 64), ((k) / 64)
  // An untiled 8x8 k_8_8_8_8 (pitch 32 texels) with each endian swap:
  // texel (3, 2) read as V.
  const u32 v32 = 0xFF302010;
  u32 p_endian[4];
  for (u32 e = 0; e < 4; e++) {
    u32 at = 0x1000 * e;
    for (u32 y = 0; y < 8; y++)
      for (u32 x = 0; x < 8; x++) *At(mem + at + 128 * y + 4 * x) = GuestWord(x == 3 && y == 2 ? v32 : 0xFFFFFFFF, e);
    struct Tex t = MakeTex(6, pmem + at, 8, 8, 1, 1, 0, 0, 32, e, 0, 2, XYZW);
    TexConst(1 + e, &t);
    p_endian[e] = NEXT;
    LoadPs(PsTex(1 + e, 1, TF_CONST, 0), 15);
    Pixel(PX(p_endian[e]), 3.5f / 8, 2.5f / 8, 0);
  }
  // Tiled 32x64: texel (13, 45) at its tiled address (its second row of
  // 32x32 macro tiles).
  u32 tiled = 0x28000;
  *At(mem + tiled + TiledAt(13, 45, 32, 2)) = GuestWord(v32, 2);
  {
    struct Tex t = MakeTex(6, pmem + tiled, 32, 64, 1, 1, 0, 1, 32, 2, 0, 2, XYZW);
    TexConst(5, &t);
  }
  u32 p_tiled = NEXT;
  LoadPs(PsTex(5, 1, TF_CONST, 0), 15);
  Pixel(PX(p_tiled), 13.5f / 32, 45.5f / 64, 0);
  // k_5_6_5, bytes swapped in halfwords: texel (5, 1) green (its missing
  // alpha the swizzle's 1).
  u32 r565 = 0x5000;
  *At(mem + r565 + 64 + 8) = GuestWord(0x07E0u << 16, 1);
  {
    struct Tex t = MakeTex(4, pmem + r565, 8, 8, 1, 1, 0, 0, 32, 1, 0, 2, 0 | 1 << 3 | 2 << 6 | 5 << 9);
    TexConst(6, &t);
  }
  u32 p_565 = NEXT;
  LoadPs(PsTex(6, 1, TF_CONST, 0), 15);
  Pixel(PX(p_565), 5.5f / 8, 1.5f / 8, 0);
  // DXT1 6x6: block (0, 0) red and blue, indices 0 1 2 3 along its first
  // row; block (1, 1) all green. DXT4_5 4x4: alpha 255 to 0 in eight
  // steps, codes 0 1 2 along its first row; colour white.
  u32 dxt1 = 0x6000, dxt5 = 0x7000;
  *At(mem + dxt1) = 0xF800 | 0x001Fu << 16;
  *At(mem + dxt1 + 4) = 0xE4;
  *At(mem + dxt1 + 256 + 8) = 0x07E0 | 0x07E0u << 16;  // rows of 32 blocks
  *At(mem + dxt1 + 256 + 12) = 0;
  *At(mem + dxt5) = 255 | 0 << 8 | 0x88 << 16;
  *At(mem + dxt5 + 4) = 0;
  *At(mem + dxt5 + 8) = 0xFFFFFFFF;
  *At(mem + dxt5 + 12) = 0;
  {
    struct Tex t1 = MakeTex(18, pmem + dxt1, 6, 6, 1, 1, 0, 0, 32, 2, 0, 2, XYZW);
    TexConst(7, &t1);
    struct Tex t5 = MakeTex(20, pmem + dxt5, 4, 4, 1, 1, 0, 0, 32, 2, 0, 2, XYZW);
    TexConst(8, &t5);
  }
  u32 p_dxt = NEXT;
  NEXT;
  NEXT;
  NEXT;
  u32 p_dxt5 = NEXT;
  NEXT;
  LoadPs(PsTex(7, 1, TF_CONST, 0), 15);
  Pixel(PX(p_dxt), 1.5f / 6, 0.5f / 6, 0);
  Pixel(PX(p_dxt + 1), 2.5f / 6, 0.5f / 6, 0);
  Pixel(PX(p_dxt + 2), 3.5f / 6, 0.5f / 6, 0);
  Pixel(PX(p_dxt + 3), 5.5f / 6, 5.5f / 6, 0);
  LoadPs(PsTex(8, 1, TF_CONST, 0), 15);
  Pixel(PX(p_dxt5), 2.5f / 4, 0.5f / 4, 0);
  Pixel(PX(p_dxt5 + 1), 1.5f / 4, 0.5f / 4, 0);
  // Packed mips: 16x16 tiled, levels 0-2 in packed tails (the base's at
  // (16, 0) of its tile, the mips' at (8, 0) and (4, 0) of theirs), each
  // level one colour; sampled at register LODs 0, 1, 2.
  u32 base = 0x8000, mips = 0x9000;
  const u32 level_colour[3] = {0xFF0000FF, 0xFF00FF00, 0xFFFF0000};
  for (u32 y = 0; y < 16; y++)
    for (u32 x = 0; x < 16; x++) *At(mem + base + TiledAt(16 + x, y, 32, 2)) = GuestWord(level_colour[0], 2);
  for (u32 lv = 1; lv < 3; lv++)
    for (u32 y = 0; y < (16u >> lv); y++)
      for (u32 x = 0; x < (16u >> lv); x++)
        *At(mem + mips + TiledAt((16u >> lv) + x, y, 32, 2)) = GuestWord(level_colour[lv], 2);
  {
    struct Tex t = MakeTex(6, pmem + base, 16, 16, 1, 1, 0, 1, 32, 2, 0, 2, XYZW);
    t.mip_max = 2;
    t.packed = 1;
    t.mip_pa = pmem + mips;
    TexConst(9, &t);
  }
  u32 p_mip = NEXT;
  NEXT;
  NEXT;
  LoadPs(PsTex(9, 1, TF_CONST | 1 << 29, 0), 15);
  for (u32 lv = 0; lv < 3; lv++) {
    SetF(0x4400, (float)lv, 0, 0, 0);
    Pixel(PX(p_mip + lv), 0.5f, 0.5f, 0);
  }
  // Address modes: a 4x4 whose texel x has red 16 (x + 1), at u 1.375:
  // repeat x 1, mirror 2, clamp to edge 3, clamp to halfway (as edge) 3.
  u32 modes = 0xA000;
  for (u32 x = 0; x < 4; x++) *At(mem + modes + 4 * x) = GuestWord(0xFF000000 | 16 * (x + 1), 2);
  u32 p_mode = NEXT;
  NEXT;
  NEXT;
  NEXT;
  const u32 clamps[4] = {0, 1, 2, 4};
  for (u32 k = 0; k < 4; k++) {
    struct Tex t = MakeTex(6, pmem + modes, 4, 4, 1, 1, 0, 0, 32, 2, 0, clamps[k], XYZW);
    TexConst(10, &t);
    LoadPs(PsTex(10, 1, TF_CONST, 0), 15);
    Pixel(PX(p_mode + k), 1.375f, 0.125f, 0);
  }
  // Filters: a 2x1, red 0 and 200, at u 0.375: point 0; linear (the
  // constant's) 50; linear by the instruction over a point constant 50.
  u32 filt = 0xB000;
  *At(mem + filt) = GuestWord(0xFF000000, 2);
  *At(mem + filt + 4) = GuestWord(0xFF0000C8, 2);
  u32 p_filter = NEXT;
  NEXT;
  NEXT;
  for (u32 k = 0; k < 3; k++) {
    struct Tex t = MakeTex(6, pmem + filt, 2, 1, 1, 1, 0, 0, 32, 2, 0, 2, XYZW); t.filter = k == 1;
    TexConst(11, &t);
    LoadPs(PsTex(11, 1, k == 2 ? (1 << 12 | 1 << 14 | 3 << 16 | 7 << 18) : TF_CONST, 0), 15);
    Pixel(PX(p_filter + k), 0.375f, 0.5f, 0);
  }
  // A texel (192, 128, 64, 254): X biased (192 to 129); swizzle WZYX;
  // exponent bias -1 (halved).
  u32 one = 0xC000;
  *At(mem + one) = GuestWord(0xFE4080C0, 2);
  u32 p_bias = NEXT;
  NEXT;
  NEXT;
  {
    struct Tex t = MakeTex(6, pmem + one, 1, 1, 1, 1, 0, 0, 32, 2, 2, 2, XYZW);
    TexConst(12, &t);
    LoadPs(PsTex(12, 1, TF_CONST, 0), 15);
    Pixel(PX(p_bias), 0.5f, 0.5f, 0);
    t.signs = 0;
    t.swizzle = 3 | 2 << 3 | 1 << 6 | 0 << 9;
    TexConst(12, &t);
    Pixel(PX(p_bias + 1), 0.5f, 0.5f, 0);
    t.swizzle = XYZW;
    t.exp = (u32)-1;
    TexConst(12, &t);
    Pixel(PX(p_bias + 2), 0.5f, 0.5f, 0);
  }
  // A cube (4x4 faces 4 KiB apart, face f red 40 (f + 1)), sampled
  // through the cube instruction's sequence: cube r1, r0.zzxy; rcp of
  // |r1.z|; mad r1.xy, r1.xy, that, 1.5; tfetchCube r1.yxw.
  u32 cube = 0x10000;
  for (u32 f = 0; f < 6; f++)
    for (u32 y = 0; y < 4; y++)
      for (u32 x = 0; x < 4; x++)
        *At(mem + cube + 0x1000 * f + 128 * y + 4 * x) = GuestWord(0xFF000000 | 40 * (f + 1), 2);
  {
    struct Tex t = MakeTex(6, pmem + cube, 4, 4, 1, 3, 0, 0, 32, 2, 0, 2, XYZW);
    TexConst(13, &t);
  }
  {
    u32 w[18];
    const struct AluOp r0zzxy = {0, 1, Swz(2, 2, 0, 1)}, r0yxzz = {0, 1, Swz(1, 0, 2, 2)}, r1 = {1, 1, Swz(0, 1, 2, 3)},
                       r1abs_z = {1 | 0x80, 1, Swz(0, 0, 0, 2)}, r2x = {2, 1, Swz(0, 0, 0, 0)},
                       c1 = {1, 0, Swz(0, 1, 2, 3)}, r3 = {3, 1, Swz(0, 1, 2, 3)};
    UcodeExec(w, 1, 5, 8);
    UcodeAlu(w + 3, 18, 1, 0xF, 0, r0zzxy, r0yxzz, r0zzxy, 50, 0, 0);       // cube r1
    UcodeAlu(w + 6, 0, 0, 0, 0, r1, r1, r1abs_z, 19, 2, 1);                // r2.x = rcp |r1.z|
    UcodeAlu(w + 9, 11, 1, 3, 0, r1, r2x, c1, 50, 0, 0);                   // mad r1.xy
    UcodeTfetch(w + 12, 1, 3, 1, 1 | 0 << 2 | 3 << 4, 13, 3, TF_CONST, 0);  // tfetchCube r3, r1.yxw
    UcodeAlu(w + 15, 2, 0, 0xF, 1, r3, r3, r3, 50, 0, 0);
    u32 pa = gq.ppool + gq.used;
    for (u32 k = 0; k < 18; k++) *At(gq.pool + gq.used + 4 * k) = w[k];
    gq.used += 80;
    LoadPs(pa, 18);
  }
  SetF(0x4404, 1.5f, 1.5f, 1.5f, 1.5f);  // c1
  const float dirs[6][3] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
  u32 p_cube = n_pixel;
  for (u32 f = 0; f < 6; f++) {
    NEXT;
    Pixel(PX(p_cube + f), dirs[f][0], dirs[f][1], dirs[f][2]);
  }
  // A 3D 4x4x4 (slices 4 KiB apart, slice z red 40 (z + 1)) and a stacked
  // 4x4 of 4 layers alike: slices and layers at W (z + 0.5) / 4; the
  // stacked one filtered between layers at W 1/4: halfway between layers
  // 0 and 1, red 60.
  u32 vol = 0x18000, stack = 0x1C000;
  for (u32 z = 0; z < 4; z++)
    for (u32 y = 0; y < 4; y++)
      for (u32 x = 0; x < 4; x++) {
        *At(mem + vol + 0x1000 * z + 128 * y + 4 * x) = GuestWord(0xFF000000 | 40 * (z + 1), 2);
        *At(mem + stack + 0x1000 * z + 128 * y + 4 * x) = GuestWord(0xFF000000 | 40 * (z + 1), 2);
      }
  u32 p_vol = n_pixel;
  n_pixel += 9;
  {
    struct Tex t3 = MakeTex(6, pmem + vol, 4, 4, 4, 2, 0, 0, 32, 2, 0, 2, XYZW);
    TexConst(14, &t3);
    struct Tex ts = MakeTex(6, pmem + stack, 4, 4, 4, 1, 1, 0, 32, 2, 0, 2, XYZW);
    TexConst(15, &ts);
    ts.vol_linear = 1;
    TexConst(16, &ts);
  }
  LoadPs(PsTex(14, 2, TF_CONST, 0), 15);
  for (u32 z = 0; z < 4; z++) Pixel(PX(p_vol + z), 0.5f, 0.5f, (z + 0.5f) / 4);
  LoadPs(PsTex(15, 2, TF_CONST, 0), 15);
  for (u32 z = 0; z < 4; z++) Pixel(PX(p_vol + 4 + z), 0.5f, 0.5f, (z + 0.5f) / 4);
  LoadPs(PsTex(16, 2, TF_CONST, 0), 15);
  Pixel(PX(p_vol + 8), 0.5f, 0.5f, 0.25f);
  // An unbound constant (type 0): zeros.
  Put(Type0(0x4800 + 6 * 17, 1));
  Put(0);
  u32 p_unbound = NEXT;
  LoadPs(PsTex(17, 1, TF_CONST, 0), 15);
  Pixel(PX(p_unbound), 0.5f, 0.5f, 0);
  // A pixel shader exporting float constant 255, the last of its 256
  // (4 KiB of them).
  u32 p_c255 = NEXT;
  {
    u32 w[6];
    const struct AluOp c255 = {255, 0, 0};
    UcodeExec(w, 1, 1, 0);
    UcodeAlu(w + 3, 2, 0, 0xF, 1, c255, c255, c255, 50, 0, 0);
    u32 pa = gq.ppool + gq.used;
    for (u32 k = 0; k < 6; k++) *At(gq.pool + gq.used + 4 * k) = w[k];
    gq.used += 32;
    SetF(0x4000 + 4 * (256 + 255), 0.2f, 0.4f, 0.6f, 1);
    LoadPs(pa, 6);
    Pixel(PX(p_c255), 0, 0, 0);
  }
  // The vertex shader sampling the endian-2 texture (its texel (3, 2), V)
  // into interpolator 0.
  {
    u32 w[18];
    const struct AluOp r1 = {1, 1, 0}, r3 = {3, 1, 0};
    UcodeExec(w, 1, 5, 7);
    UcodeVfetch(w + 3, 1, 0, 57, 7, 0);
    UcodeVfetch(w + 6, 2, 0, 38, 7, 3);
    UcodeTfetch(w + 9, 1, 3, 2, 0 | 1 << 2 | 2 << 4, 3, 1, TF_CONST, 0);
    UcodeAlu(w + 12, 2, 62, 0xF, 1, r1, r1, r1, 50, 0, 0);
    UcodeAlu(w + 15, 2, 0, 0xF, 1, r3, r3, r3, 50, 0, 0);
    u32 pa = gq.ppool + gq.used;
    for (u32 k = 0; k < 18; k++) *At(gq.pool + gq.used + 4 * k) = w[k];
    gq.used += 80;
    Put(Type3(0x27, 2));
    Put(pa);
    Put(18);
  }
  PixelShader(0);
  u32 p_vertex = NEXT;
  Pixel(PX(p_vertex), 3.5f / 8, 2.5f / 8, 0);
  VertexShader(0);
  // A resolve's destination sampled before and after the resolve in one
  // frame: a 64x64 tiled 8_8_8_8 (as the resolve writes it) holding A at
  // (5, 5), then target 0x220 cleared to C resolved over it.
  u32 rtt = 0x20000;
  *At(mem + rtt + TiledAt(5, 5, 64, 2)) = 0xFFA0A0A0;
  {
    struct Tex t = MakeTex(6, pmem + rtt, 64, 64, 1, 1, 0, 1, 64, 2, 0, 2, XYZW);
    TexConst(18, &t);
  }
  u32 p_rtt = NEXT;
  NEXT;
  LoadPs(PsTex(18, 1, TF_CONST, 0), 15);
  Pixel(PX(p_rtt), 5.5f / 64, 5.5f / 64, 0);
  Set(0x2001, 0x220);
  Set(0x231E, 0xFF0C0B0A);
  Resolve(0x100, pmem + 0x30000, 6 << 7 | 2, 0);
  Resolve(0, pmem + rtt, 6 << 7 | 2, 0);
  Set(0x2001, 0x200);
  Pixel(PX(p_rtt + 1), 5.5f / 64, 5.5f / 64, 0);
  // A 1x1 rewritten by the CPU: sampled, then (after a swap) sampled again.
  u32 cpu = 0x25000;
  *At(mem + cpu) = GuestWord(0xFF111111, 2);
  {
    struct Tex t = MakeTex(6, pmem + cpu, 1, 1, 1, 1, 0, 0, 32, 2, 0, 2, XYZW);
    TexConst(19, &t);
  }
  u32 p_cpu = NEXT;
  NEXT;
  LoadPs(PsTex(19, 1, TF_CONST, 0), 15);
  Pixel(PX(p_cpu), 0.5f, 0.5f, 0);
  Kick();
  *At(mem + cpu) = GuestWord(0xFF222222, 2);
  {
    u32 fetch[6] = {1u << 31 | 2 << 22, (mem + 0xF0000) | 2 << 6 | 6, 63 | 63 << 13,
                    (0 | 1 << 3 | 2 << 6 | 3 << 9) << 1, 0, 0},
        front = mem + 0xF0000, format = 6, color_space = 0, width = 64, height = 64;
    VdSwap10(&gq.rb[gq.w], fetch, 0, 0, 0, &front, &format, &color_space, &width, &height);
    gq.w += 64;
  }
  LoadPs(PsTex(19, 1, TF_CONST, 0), 15);
  Pixel(PX(p_cpu + 1), 0.5f, 0.5f, 0);

  // Signed and depth into the 32_32_FLOAT target: X, Y signed of (0x81,
  // 0x40, 0, 0x7F): -1 and 64/127; a k_24_8 texel 0xC00000: (d + d >> 23)
  // / 2^24, 0.75 + 2^-24.
  Set(0x2001, 0x210 | 15 << 16);
  Set(0x231E, 0);
  Set(0x231F, 0);
  Resolve(0x100, dest_f, 37 << 7 | 2, 1);
  u32 sgn = 0x26000, dep = 0x27000;
  *At(mem + sgn) = GuestWord(0x7F004081, 2);
  *At(mem + dep) = GuestWord(0xC0000000, 2);
  {
    struct Tex ts = MakeTex(6, pmem + sgn, 1, 1, 1, 1, 0, 0, 32, 2, 1 | 1 << 2, 2, XYZW);
    TexConst(20, &ts);
    struct Tex td = MakeTex(22, pmem + dep, 1, 1, 1, 1, 0, 0, 32, 2, 0, 2, XYZW);
    TexConst(21, &td);
  }
  LoadPs(PsTex(20, 1, TF_CONST, 0), 15);
  Pixel(0, 0, 0.5f, 0.5f, 0);
  LoadPs(PsTex(21, 1, TF_CONST, 0), 15);
  Pixel(1, 0, 0.5f, 0.5f, 0);
  Resolve(0, dest_f, 37 << 7 | 2, 1);
  Set(0x2001, 0x200);
  Resolve(0, dest, 6 << 7 | 2, 0);
  PixelShader(0);
  Kick();

  volatile u32* t = At(mem + 0xF0000);
#define TX(k) t[TiledAt(PX(k), 64, 2) / 4]
  for (u32 e = 0; e < 4; e++)
    if (TX(p_endian[e]) != v32) DbgPrint("FAIL line %d: endian %u: %08x\n", __LINE__, e, TX(p_endian[e]));
  CHECK(TX(p_tiled) == v32 && TX(p_565) == 0xFF00FF00);
  CHECK(TX(p_dxt) == 0xFFFF0000 && TX(p_dxt + 1) == 0xFF5500AA && TX(p_dxt + 2) == 0xFFAA0055 &&
        TX(p_dxt + 3) == 0xFF00FF00);
  CHECK(TX(p_dxt5) == 0xDAFFFFFF && TX(p_dxt5 + 1) == 0x00FFFFFF);
  CHECK(TX(p_mip) == level_colour[0] && TX(p_mip + 1) == level_colour[1] && TX(p_mip + 2) == level_colour[2]);
  CHECK(TX(p_mode) == 0xFF000020 && TX(p_mode + 1) == 0xFF000030 && TX(p_mode + 2) == 0xFF000040 &&
        TX(p_mode + 3) == 0xFF000040);
  CHECK(TX(p_filter) == 0xFF000000 && TX(p_filter + 1) == 0xFF000032 && TX(p_filter + 2) == 0xFF000032);
  CHECK(TX(p_bias) == 0xFE408081 && TX(p_bias + 1) == 0xC08040FE && TX(p_bias + 2) == 0x7F204060);
  for (u32 f = 0; f < 6; f++)
    if (TX(p_cube + f) != (0xFF000000 | 40 * (f + 1)))
      DbgPrint("FAIL line %d: cube face %u: %08x\n", __LINE__, f, TX(p_cube + f));
  for (u32 z = 0; z < 8; z++)
    if (TX(p_vol + z) != (0xFF000000 | 40 * (z % 4 + 1)))
      DbgPrint("FAIL line %d: %s %u: %08x\n", __LINE__, z < 4 ? "3D slice" : "stacked layer", z % 4, TX(p_vol + z));
  CHECK(TX(p_vol + 8) == 0xFF00003C);
  CHECK(TX(p_unbound) == 0);
  CHECK(TX(p_c255) == 0xFF996633);
  CHECK(TX(p_vertex) == v32);
  CHECK(TX(p_rtt) == 0xFFA0A0A0 && TX(p_rtt + 1) == 0xFF0C0B0A);
  CHECK(TX(p_cpu) == 0xFF111111 && TX(p_cpu + 1) == 0xFF222222);
  volatile u32* tf = At(mem + 0xF4000);
  float g;
  u32 gbits = tf[TiledAt(0, 0, 64, 3) / 4 + 1];
  memcpy(&g, &gbits, 4);
  CHECK(tf[TiledAt(0, 0, 64, 3) / 4] == 0xBF800000 && g > 64.0f / 127 - 1e-4f && g < 64.0f / 127 + 1e-4f);
  CHECK(tf[TiledAt(1, 0, 64, 3) / 4] == 0x3F400001);
  MmFreePhysicalMemory(0, mem);
#undef TX
#undef NEXT
#undef PX
}

// A mark of the GPU checks for the test program (title_main.cpp): since
// the last mark, resolves landed `landings` times, `reads` of them for
// the GPU's reads of memory a resolve wrote, and draws loaded `loads`
// host targets; Mark(-1, ...) only sets one. MarkStencil checks too that
// loads drew `load_bits` stencil bits (unless negative) and stores
// `store_bits`. From an idle GPU (after Kick).
static void MarkStencil(int landings, u32 reads, u32 loads, int load_bits, u32 store_bits) {
  uint16_t vibration[2] = {
      (uint16_t)(landings < 0 ? 0x8000 : 0x8000 | (u32)landings << 10 | reads << 5 | loads),
      (uint16_t)(load_bits < 0 ? 0 : 0x8000 | store_bits << 8 | (u32)load_bits)};
  XamInputSetState(0, 0, vibration);
  vibration[0] = vibration[1] = 0;
  XamInputSetState(0, 0, vibration);
}
static void Mark(int landings, u32 reads, u32 loads) { MarkStencil(landings, reads, loads, -1, 0); }

// Resolves into textures the cache holds, sampled before and after: those
// whose level 0 is the copy's destination, in a format that decodes its
// words as they are, take its texels on the device; others decode the
// memory again once it has landed. Draws land resolves only where they
// read what one wrote. Resolve clears that cover a target keep it from
// loading again, with its stencil known.
static void InPlace(void) {
  u32 mem = MmAllocatePhysicalMemoryEx(0, 0x100000, kPageReadWrite, 0, 0, 0x1000), pmem = MmGetPhysicalAddress(mem);
  for (u32 k = 0; k < 0x100000; k += 4) *At(mem + k) = 0;
  gq.used = 0;
  // Each clearing resolve copies to a 16 KiB area of its own (an overlap
  // with another still on the device would land it).
  u32 out = pmem, out_f = pmem + 0x4000, slot = pmem + 0x80000;
#define SCRATCH (slot += 0x4000)
  u32 n_pixel = 0, n_float = 0;
#define NEXT (n_pixel++)
#define PX(k) ((k) % 64), ((k) / 64)
#define FNEXT (n_float++)
#define FX(k) ((k) % 16), ((k) / 16)
  // The output targets: 8_8_8_8 at 0x200 (cleared, all of it: kept), and
  // 32_32_FLOAT at 0x210.
  Set(0x2000, 64);
  Set(0x2001, 0x200);
  Set(0x231E, 0);
  Set(0x231F, 0);
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 0);
  Set(0x2001, 0x210 | 15 << 16);
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 1);
  Set(0x2001, 0x200);
  Kick();
  gq.used = 0;
  Mark(-1, 0, 0);

  // Two resolves of 8_8_8_8 target 0x300 into a 64x64 texture, the 16x16
  // corner in A, then all of it in B: sampled in place, nothing lands but
  // the fence's and the second resolve's (over the first's range).
  u32 t1 = pmem + 0x20000;
  {
    struct Tex t = MakeTex(6, t1, 64, 64, 1, 1, 0, 1, 64, 2, 0, 2, XYZW);
    TexConst(1, &t);
  }
  u32 p_two = NEXT;
  NEXT, NEXT, NEXT, NEXT;
  LoadPs(PsTex(1, 1, TF_CONST, 0), 15);
  Pixel(PX(p_two), 5.5f / 64, 5.5f / 64, 0);
  Set(0x2001, 0x300);
  Set(0x231E, 0xFF0000FF);
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 0);
  Resolve(0, t1, 6 << 7 | 2, 1);
  Set(0x2001, 0x200);
  Pixel(PX(p_two + 1), 5.5f / 64, 5.5f / 64, 0);
  Pixel(PX(p_two + 2), 20.5f / 64, 20.5f / 64, 0);
  Set(0x2001, 0x300);
  Set(0x231E, 0xFF00FF00);
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 0);
  Resolve(0, t1, 6 << 7 | 2, 0);
  Set(0x2001, 0x200);
  Pixel(PX(p_two + 3), 20.5f / 64, 20.5f / 64, 0);
  Pixel(PX(p_two + 4), 5.5f / 64, 5.5f / 64, 0);
  Kick();
  gq.used = 0;
  Mark(2, 0, 0);

  // 4x MSAA: pixel (0, 0) of 8_8_8_8 target 0x380 with samples 0 and 1 in
  // (1, 0.5, 0.25, 1), 2 and 3 cleared to (0, 0, 0, 1); resolved averaged
  // with exponent bias -1 (0x80102040; (1, 0) 0x80000000), and red and
  // blue swapped too (0x80402010), into textures sampled before; then a
  // coherency request (COHER_STATUS_HOST, which WAIT_REG_MEM sees done)
  // and another resolve: the fence lands them all at once, the samples
  // after the request are the resolves'.
  u32 t2 = pmem + 0x24000, t3 = pmem + 0x28000;
  {
    struct Tex t = MakeTex(6, t2, 64, 64, 1, 1, 0, 1, 64, 2, 0, 2, XYZW);
    TexConst(2, &t);
    t.pa = t3;
    TexConst(3, &t);
  }
  u32 p_avg = NEXT;
  NEXT, NEXT, NEXT, NEXT;
  LoadPs(PsTex(2, 1, TF_CONST, 0), 15);
  Pixel(PX(p_avg), 0.5f / 64, 0.5f / 64, 0);
  LoadPs(PsTex(3, 1, TF_CONST, 0), 15);
  Pixel(PX(p_avg + 1), 0.5f / 64, 0.5f / 64, 0);
  Set(0x2000, 64 | 2 << 16);
  Set(0x2001, 0x380);
  Set(0x231E, 0xFF000000);
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 0);
  PixelShader(0);
  const float c_avg[4] = {1, 0.5f, 0.25f, 1};
  Rect(-0.5f, -0.5f, 0, 0.5f, 0, c_avg);
  Resolve(6 << 4, t2, 63u << 16 | 6 << 7 | 2, 1);
  Resolve(6 << 4, t3, 1u << 24 | 63u << 16 | 6 << 7 | 2, 1);
  Set(0x0A31, 0x10);
  Put(Type3(0x3C, 5));
  Put(3);
  Put(0x0A31);
  Put(0);
  Put(0xFFFFFFFF);
  Put(0x100);
  Resolve(0, SCRATCH, 6 << 7 | 2, 1);
  Set(0x2000, 64);
  Set(0x2001, 0x200);
  LoadPs(PsTex(2, 1, TF_CONST, 0), 15);
  Pixel(PX(p_avg + 2), 0.5f / 64, 0.5f / 64, 0);
  Pixel(PX(p_avg + 3), 1.5f / 64, 0.5f / 64, 0);
  LoadPs(PsTex(3, 1, TF_CONST, 0), 15);
  Pixel(PX(p_avg + 4), 0.5f / 64, 0.5f / 64, 0);
  Kick();
  gq.used = 0;
  Mark(1, 0, 1);

  // Each texture format a resolve writes, 16x16 at a 64-texel pitch, the
  // endian swaps in turn: one level (in place) and, at the same address,
  // with a second level (decoded again, after its first sample lands the
  // resolves), from 16_16_16_16_FLOAT target 0x320 cleared to (0.25,
  // 0.5, 0.75, 1) (the depth formats from depth target 0x340, depth
  // 0.75 + 2^-24, stencil 0x55): both sampled into the 32_32_FLOAT target,
  // X and Y then Z and W, alike. 16_16_16_16 into a texture of 8_8_8_8
  // and 8_8_8_8 with another endian swap: refused, decoded again.
  static const u32 kWrites[17][3] = {
      // texture format, the resolve's, endian swap
      {6, 6, 0},   {14, 14, 1}, {50, 50, 2}, {7, 7, 3},   {54, 54, 0}, {25, 25, 1}, {26, 26, 2}, {31, 31, 3},
      {28, 31, 0}, {32, 32, 1}, {29, 32, 2}, {36, 36, 3}, {37, 37, 0}, {22, 22, 1}, {23, 22, 2}, {6, 26, 2},
      {6, 6, 3},
  };
  u32 p_fmt = FNEXT, ps_fmt[4];
  for (u32 c = 0; c < 4; c++) ps_fmt[c] = PsTex(4 + c, 1, TF_CONST, 0);
  for (u32 k = 1; k < 17 * 4; k++) FNEXT;
  Set(0x2001, 0x210 | 15 << 16);
  for (int after = 0; after < 2; after++) {
    if (after) {
      Set(0x2001, 0x320 | 7 << 16);
      Set(0x231E, 0x38003400);
      Set(0x231F, 0x3C003A00);
      Resolve(0x100, SCRATCH, 6 << 7 | 2, 0);
      Set(0x2002, 0x340);
      Set(0x231D, 0xC0000055);
      Resolve(0x200 | 4, SCRATCH, 6 << 7 | 2, 0);
      for (u32 k = 0; k < 17; k++) {
        u32 at = t1 + 0x10000 + 0x2000 * k, e = k == 16 ? 2 : kWrites[k][2];
        Resolve(kWrites[k][0] == 22 || kWrites[k][0] == 23 ? 4 : 0, at, kWrites[k][1] << 7 | e, 1);
      }
      Set(0x2001, 0x210 | 15 << 16);
    }
    for (u32 k = 0; k < 17; k++) {
      u32 at = t1 + 0x10000 + 0x2000 * k;
      struct Tex t = MakeTex(kWrites[k][0], at, 16, 16, 1, 1, 0, 1, 64, kWrites[k][2], 0, 2, XYZW);
      TexConst(4, &t);
      t.swizzle = 2 | 3 << 3 | 0 << 6 | 1 << 9;
      TexConst(5, &t);
      t.mip_max = 1;
      t.mip_pa = pmem + 0xC0000 + 0x1000 * k;
      t.swizzle = XYZW;
      TexConst(6, &t);
      t.swizzle = 2 | 3 << 3 | 0 << 6 | 1 << 9;
      TexConst(7, &t);
      for (u32 c = 0; c < 4; c++) {
        LoadPs(ps_fmt[c], 15);
        Pixel(FX(p_fmt + 4 * k + c), 5.5f / 16, 5.5f / 16, 0);
      }
    }
  }
  // A 64x64 8_8_8_8 at a 128-texel pitch, all of target 0x320 resolved
  // there at a 64-texel pitch: refused, decoded again (texel (40, 40),
  // past what the copy wrote, as before: 0), alike with a second level.
  u32 tp = pmem + 0x70000, p_pitch = FNEXT;
  FNEXT;
  for (int after = 0; after < 2; after++) {
    if (after) {
      Set(0x2001, 0x320 | 7 << 16);
      Resolve(0, tp, 6 << 7 | 2, 0);
      Set(0x2001, 0x210 | 15 << 16);
    }
    struct Tex t = MakeTex(6, tp, 64, 64, 1, 1, 0, 1, 128, 2, 0, 2, XYZW);
    TexConst(4, &t);
    t.mip_max = 1;
    t.mip_pa = pmem + 0xD8000;
    TexConst(6, &t);
    LoadPs(ps_fmt[0], 15);
    Pixel(FX(p_pitch), 40.5f / 64, 40.5f / 64, 0);
    LoadPs(ps_fmt[2], 15);
    Pixel(FX(p_pitch + 1), 40.5f / 64, 40.5f / 64, 0);
  }
  // A 2_10_10_10 texel decoded on the CPU, red 820: 16-bit unorm 52530
  // (820 / 1023 rounded to nearest; in floats it rounds up).
  u32 t10 = pmem + 0x5C000;
  *At(mem + 0x5C000) = GuestWord(820, 2);
  {
    struct Tex t = MakeTex(7, t10, 1, 1, 1, 1, 0, 0, 32, 2, 0, 2, XYZW);
    TexConst(8, &t);
  }
  u32 p_10 = FNEXT;
  LoadPs(PsTex(8, 1, TF_CONST, 0), 15);
  Pixel(FX(p_10), 0.5f, 0.5f, 0);
  Kick();
  gq.used = 0;
  Mark(2, 2, 1);
  Resolve(0, out_f, 37 << 7 | 2, 1);
  Set(0x2001, 0x200);
  Kick();
  gq.used = 0;
  Mark(1, 0, 0);
  volatile u32* tf = At(mem + 0x4000);
#define TF(k, c) tf[TiledAt(FX(k), 64, 3) / 4 + (c)]
  for (u32 k = 0; k < 17; k++)
    for (u32 c = 0; c < 4; c++)
      if (TF(p_fmt + 4 * k + c, 0) != TF(p_fmt + 4 * k + (c ^ 2), 0) ||
          TF(p_fmt + 4 * k + c, 1) != TF(p_fmt + 4 * k + (c ^ 2), 1))
        DbgPrint("FAIL line %d: format %u, %s: %08x %08x / %08x %08x\n", __LINE__, kWrites[k][0],
                 c & 1 ? "Z W" : "X Y", TF(p_fmt + 4 * k + c, 0), TF(p_fmt + 4 * k + c, 1),
                 TF(p_fmt + 4 * k + (c ^ 2), 0), TF(p_fmt + 4 * k + (c ^ 2), 1));
  // The values: 8_8_8_8 (0.25 0.5 0.75 1 packed: 64 128 191 255),
  // 16_16_16_16_FLOAT and 32_FLOAT as they are, k_24_8.
  float f8;
  u32 b8 = TF(p_fmt, 0);
  memcpy(&f8, &b8, 4);
  CHECK(f8 > 63.5f / 255 && f8 < 64.5f / 255 && TF(p_fmt + 1, 1) == FloatBits(1));
  CHECK(TF(p_pitch, 0) == 0 && TF(p_pitch + 1, 0) == 0);
  CHECK(TF(p_fmt + 4 * 9, 0) == FloatBits(0.25f) && TF(p_fmt + 4 * 9 + 1, 0) == FloatBits(0.75f));
  CHECK(TF(p_fmt + 4 * 11, 0) == FloatBits(0.25f) && TF(p_fmt + 4 * 13, 0) == 0x3F400001);

  // A texture updated in place, landed, then rewritten by the CPU: the
  // next frame samples the CPU's texel.
  *At(mem + 0x20000 + TiledAt(5, 5, 64, 2)) = 0xFF333333;
  {
    u32 fetch[6] = {1u << 31 | 2 << 22, (mem + 0xF0000) | 2 << 6 | 6, 63 | 63 << 13,
                    (0 | 1 << 3 | 2 << 6 | 3 << 9) << 1, 0, 0},
        front = mem + 0xF0000, format = 6, color_space = 0, width = 64, height = 64;
    VdSwap10(&gq.rb[gq.w], fetch, 0, 0, 0, &front, &format, &color_space, &width, &height);
    gq.w += 64;
  }
  u32 p_cpu = NEXT;
  LoadPs(PsTex(1, 1, TF_CONST, 0), 15);
  Pixel(PX(p_cpu), 5.5f / 64, 5.5f / 64, 0);

  // A point whose vertex is a resolve's texels: 32_FLOAT target 0x360
  // cleared to 8 and resolved (16x16) into the vertex buffer: x, y, z 8,
  // colour (8, 8, 8, 8): pixel (8, 8) white. Its fetch lands the resolve.
  u32 vb = pmem + 0xE0000;
  Set(0x2001, 0x360 | 14 << 16);
  Set(0x231E, FloatBits(8));
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 0);
  Resolve(0, vb, 36 << 7 | 2, 1);
  Set(0x2001, 0x200);
  PixelShader(0);
  Set(0x2280, 8 << 16 | 8);
  DrawAuto(1, vb, 1);
  Kick();
  gq.used = 0;
  Mark(1, 1, 0);

  // Clears: target 0x3A0 drawn, then cleared in its corner (16x16), and
  // drawn again: it loads; target 0x3C0 drawn, then all of 0x3A0 cleared
  // (kept, no load) and both drawn again: neither loads.
  u32 cl = pmem + 0x60000, cl2 = pmem + 0x64000;
  Set(0x2001, 0x3A0);
  Rect(0, 0, 64, 64, 0, kRed);
  Set(0x2001, 0x3C0);
  Rect(0, 0, 64, 64, 0, kGreen);
  Kick();
  gq.used = 0;
  Mark(-1, 0, 0);
  Set(0x2001, 0x3A0);
  Set(0x231E, 0xFFFF0000);
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 1);
  Rect(40, 40, 48, 48, 0, kWhite);
  Kick();
  gq.used = 0;
  Mark(1, 0, 1);
  Set(0x231E, 0xFF00FF00 | 0x80);
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 0);
  Rect(40, 40, 48, 48, 0, kWhite);
  Set(0x2001, 0x3C0);
  Rect(0, 0, 8, 8, 0, kBlue);
  Kick();
  gq.used = 0;
  Mark(1, 0, 0);
  Set(0x2001, 0x3A0);
  Resolve(0, cl, 6 << 7 | 2, 0);
  Set(0x2001, 0x3C0);
  Resolve(0, cl2, 6 << 7 | 2, 0);
  Set(0x2001, 0x200);

  // Depth target 0x3E0 (D24S8) with target 0x200 (no colour written):
  // drawn (depth 0.75, always), cleared all (depth 1, stencil 0x5A: kept,
  // known), drawn at depth 0.5 where less (8-24) without stencil, resolved
  // (stencil known: 0x8000005A there, 0xFFFFFF5A around); then 0x33
  // replaced at (32-40) where the stencil passes, resolved: per bit again.
  u32 dd = pmem + 0x68000, dd2 = pmem + 0x6C000;
  Set(0x2104, 0);
  Set(0x2002, 0x3E0);
  Set(0x2200, 2 | 4 | 7 << 4);
  Rect(0, 0, 64, 64, 0.75f, kBlack);
  Set(0x231D, 0xFFFFFF5A);
  Resolve(0x200 | 4, SCRATCH, 6 << 7 | 2, 0);
  Set(0x2200, 2 | 4 | 1 << 4);
  Rect(8, 8, 24, 24, 0.5f, kBlack);
  Set(0x2200, 0);
  Resolve(4, dd, 6 << 7 | 2, 0);
  Set(0x210D, 0xFFFF33);
  Set(0x2200, 1 | 7 << 8 | 2 << 14);
  Rect(32, 32, 40, 40, 0, kBlack);
  Set(0x2200, 0);
  Resolve(4, dd2, 6 << 7 | 2, 0);
  Set(0x2002, 0x500);
  Set(0x2104, 0xF);

  Resolve(0, out, 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  volatile u32* t = At(mem);
#define TX(k) t[TiledAt(PX(k), 64, 2) / 4]
  CHECK(TX(p_two) == 0 && TX(p_two + 1) == 0xFF0000FF && TX(p_two + 2) == 0);
  CHECK(TX(p_two + 3) == 0xFF00FF00 && TX(p_two + 4) == 0xFF00FF00);
  CHECK(TX(p_avg) == 0 && TX(p_avg + 1) == 0);
  CHECK(TX(p_avg + 2) == 0x80102040 && TX(p_avg + 3) == 0x80000000 && TX(p_avg + 4) == 0x80402010);
  CHECK(*At(mem + 0x24000) == 0x80102040 && *At(mem + 0x28000) == 0x80402010);
  CHECK(TX(p_cpu) == 0xFF333333);
  CHECK(t[TiledAt(8, 8, 64, 2) / 4] == 0xFFFFFFFF);
  float f10;
  u32 b10 = TF(p_10, 0);
  memcpy(&f10, &b10, 4);
  CHECK(f10 > 52529.5f / 65535 && f10 < 52530.5f / 65535);
  volatile u32* c1 = At(mem + 0x60000);
  volatile u32* c2 = At(mem + 0x64000);
  CHECK(c1[TiledAt(4, 4, 64, 2) / 4] == (0xFF00FF00 | 0x80) && c1[TiledAt(44, 44, 64, 2) / 4] == 0xFFFFFFFF);
  CHECK(c2[TiledAt(4, 4, 64, 2) / 4] == 0xFFFF0000 && c2[TiledAt(20, 20, 64, 2) / 4] == 0xFF00FF00);
  volatile u32* d1 = At(mem + 0x68000);
  volatile u32* d2 = At(mem + 0x6C000);
  CHECK(d1[TiledAt(16, 16, 64, 2) / 4] == 0x8000005A && d1[TiledAt(40, 4, 64, 2) / 4] == 0xFFFFFF5A);
  CHECK(d2[TiledAt(36, 36, 64, 2) / 4] == 0xFFFFFF33 && d2[TiledAt(16, 16, 64, 2) / 4] == 0x8000005A);
  // EVENT_WRITE_EXT after a resolve, then a wait (SCRATCH_REG1) that holds
  // the GPU: the extents' words are written, and the resolve's texels are
  // in memory before them (texel (1, 0) of target 0x200, 0xFF0000FF).
  *At(mem + 0x57000) = 0xAAAAAAAA;
  Set(0x0579, 0);
  Resolve(0, pmem + 0x56000, 6 << 7 | 2, 1);
  Put(Type3(0x5A, 2));
  Put(0);
  Put(pmem + 0x57000);
  Put(Type3(0x3C, 5));
  Put(3);
  Put(0x579);
  Put(1);
  Put(0xFFFFFFFF);
  Put(0x100);
  gq.rb[gq.w++] = Type3(0x3F, 2);
  gq.rb[gq.w++] = gq.pib;
  gq.rb[gq.w++] = gq.n;
  *Reg(0x1C5) = gq.w;
  CHECK(WaitFor(At(mem + 0x57000), 0x400));
  CHECK(*At(mem + 0x56000 + TiledAt(1, 0, 64, 2)) == 0xFF0000FF);
  *Reg(0x579) = 1;
  gq.n = 0;
  Kick();
  MmFreePhysicalMemory(0, mem);
#undef TX
#undef TF
#undef NEXT
#undef PX
#undef FNEXT
#undef FX
#undef SCRATCH
}

// Depth targets' stencil on the host: loaded from the EDRAM's words only
// when a draw tests it (where the EDRAM's tiles are known to hold one
// value, with the depth, drawing no stencil bit), and stored only where a
// draw wrote it (else the EDRAM's words keep their stencil bytes). Two
// surfaces over depth tiles 0x780-0x787, each loading what the other
// left: A, 4x MSAA 40 pixels wide (a tile's 80 samples a row: its clears
// cover whole tiles), colour at 0x7D0; B, 2x MSAA 64 pixels wide (part of
// each tile), colour at 0x7C0. A pixel (x, y) is depth sample (2x, 2y) of
// A's, (x, 2y) of B's.
static void StencilLoads(void) {
  u32 mem = MmAllocatePhysicalMemoryEx(0, 0x80000, kPageReadWrite, 0, 0, 0x1000), pmem = MmGetPhysicalAddress(mem);
  for (u32 k = 0; k < 0x80000; k += 4) *At(mem + k) = 0;
  gq.used = 0;
  u32 slot = pmem + 0x40000;
#define SCRATCH (slot += 0x4000)
#define OUT(k) (pmem + 0x4000 * (k))
  const u32 surface_a = 40 | 2 << 16, surface_b = 64 | 1 << 16;
  // (a) A's depth cleared to stencil 0xA5, B's colour to 0; B white where
  // the stencil is 0xA5 (8-16, 8-16), and where it is 0xA4 (24-32): its
  // depth loads tiles known to hold 0xA5, with no stencil bit.
  Set(0x2000, surface_a);
  Set(0x2002, 0x780);
  Set(0x231D, 0xFFFFFFA5);
  Resolve(0x200 | 4, SCRATCH, 6 << 7 | 2, 0);
  Set(0x2000, surface_b);
  Set(0x2001, 0x7C0);
  Set(0x231E, 0);
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  Mark(-1, 0, 0);
  PixelShader(0);
  Set(0x2200, 1 | 2 << 8);
  Set(0x210D, 0xA5 | 0xFF << 8);
  Rect(8, 8, 16, 16, 0, kWhite);
  Set(0x210D, 0xA4 | 0xFF << 8);
  Rect(24, 8, 32, 16, 0, kWhite);
  Set(0x2200, 0);
  Resolve(0, OUT(0), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 2, 0, 0);
  // (b) B's stencil replaced by 0x3C at rows 32-48 (its pixels 0-64),
  // stored a bit at a time (8); A's colour cleared, its depth loaded
  // without its stencil, which A's first stencil test loads a bit at a
  // time (8); red where the stencil is 0xA5, green where 0x3C (A's pixels
  // 32-40 are past B's 64 columns: red). A's store keeps the EDRAM's
  // stencil bytes (none).
  Set(0x2200, 1 | 7 << 8 | 2 << 14);
  Set(0x210D, 0x3C | 0xFF << 16);
  Rect(0, 32, 64, 48, 0, kBlack);
  Set(0x2200, 0);
  Set(0x2000, surface_a);
  Set(0x2001, 0x7D0);
  Resolve(0x100, SCRATCH, 6 << 7 | 2, 0);
  Set(0x2200, 1 | 2 << 8);
  Set(0x210D, 0xA5 | 0xFF << 8);
  Rect(0, 0, 40, 64, 0, kRed);
  Set(0x210D, 0x3C | 0xFF << 8);
  Rect(0, 0, 40, 64, 0, kGreen);
  Set(0x2200, 0);
  Resolve(0, OUT(1), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 2, 8, 8);
  // (c) A's depth cleared to 0xA5 again (all its tiles known); colour
  // target 0x780 of B in 0x33 at rows 0-8, over them: B's depth loads,
  // and its stencil test draws each bit (8); white where the stencil is
  // 0x33 (rows 0-8), in 0x7C0. No store draws any (none written).
  Set(0x231D, 0xFFFFFFA5);
  Resolve(0x200 | 4, SCRATCH, 6 << 7 | 2, 0);
  Set(0x2000, surface_b);
  Set(0x2001, 0x780);
  const float c_33[4] = {0.2f, 0.2f, 0.2f, 0.2f};
  Rect(0, 0, 64, 8, 0, c_33);
  Set(0x2001, 0x7C0);
  Set(0x2200, 1 | 2 << 8);
  Set(0x210D, 0x33 | 0xFF << 8);
  Rect(0, 0, 64, 16, 0, kWhite);
  Set(0x2200, 0);
  Resolve(0, OUT(2), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 2, 8, 0);
  // (d) B's depth cleared to 0x5A: its 64 columns, part of each tile; A's
  // stencil test loads each bit (8): blue where the stencil is 0x5A,
  // white where 0xA5 (A's pixels 32-40, below the 0x33 rows).
  Set(0x231D, 0xFFFFFF5A);
  Resolve(0x200 | 4, SCRATCH, 6 << 7 | 2, 0);
  Set(0x2000, surface_a);
  Set(0x2001, 0x7D0);
  Set(0x2200, 1 | 2 << 8);
  Set(0x210D, 0x5A | 0xFF << 8);
  Rect(0, 0, 40, 64, 0, kBlue);
  Set(0x210D, 0xA5 | 0xFF << 8);
  Rect(0, 0, 40, 64, 0, kWhite);
  Set(0x2200, 0);
  Resolve(0, OUT(3), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 1, 8, 0);
  // (e) A's depth cleared (depth 1, stencil 0xA5), 0x3C replaced at its
  // rows 32-48, stored. Then B, without a stencil test: its depth loads
  // with no stencil bit, depth 0.5 drawn at (8-24, 8-24), stored with
  // none either, A's depth resolved: B's depth over A's stencil bytes
  // (A's pixel (8, 12): 0x800000A5; (8, 40): 0xFFFFFF3C). A's stencil
  // tests then load its stencil (8): green where 0x3C, red where 0xA5.
  Set(0x231D, 0xFFFFFFA5);
  Resolve(0x200 | 4, SCRATCH, 6 << 7 | 2, 0);
  Set(0x2200, 1 | 7 << 8 | 2 << 14);
  Set(0x210D, 0x3C | 0xFF << 16);
  Rect(0, 32, 40, 48, 0, kBlack);
  Set(0x2200, 0);
  Resolve(0, SCRATCH, 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  Mark(-1, 0, 0);
  Set(0x2000, surface_b);
  Set(0x2001, 0x7C0);
  Set(0x2200, 2 | 4 | 7 << 4);
  Rect(8, 8, 24, 24, 0.5f, kBlack);
  Set(0x2200, 0);
  Set(0x2000, surface_a);
  Resolve(4, OUT(4), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 1, 0, 0);
  Set(0x2001, 0x7D0);
  Set(0x2200, 1 | 2 << 8);
  Set(0x210D, 0x3C | 0xFF << 8);
  Rect(0, 0, 40, 64, 0, kGreen);
  Set(0x210D, 0xA5 | 0xFF << 8);
  Rect(0, 0, 40, 64, 0, kRed);
  Set(0x2200, 0);
  Resolve(0, OUT(5), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 1, 8, 0);
  // (f) B's depth loads without its stencil; depth 0.25 drawn at (32-48,
  // 8-24); then blue where the stencil is 0xA5 and the depth less than
  // 0.375: the test loads the stencil (8), the depth as drawn (not at
  // 32-48, nor at rows 32-48 where the stencil is 0x3C).
  Set(0x2000, surface_b);
  Set(0x2001, 0x7C0);
  Set(0x2200, 2 | 4 | 7 << 4);
  Rect(32, 8, 48, 24, 0.25f, kBlack);
  Set(0x2200, 1 | 2 | 1 << 4 | 2 << 8);
  Rect(0, 0, 64, 64, 0.375f, kBlue);
  Set(0x2200, 0);
  Resolve(0, OUT(6), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 1, 8, 0);
  // (g) A's depth loads without its stencil; 0x77 replaced at its rows
  // 0-16 (the test loads the stencil first: 8); stored a bit at a time
  // (8) by the resolve of A's depth (0xFFFFFF77 at (6, 6); B's depth 0.5
  // over 0xA5 at (6, 20)). B's test then loads it (8): white where it is
  // 0x77, at B's rows 0-16.
  Set(0x2000, surface_a);
  Set(0x2001, 0x7D0);
  Set(0x2200, 1 | 7 << 8 | 2 << 14);
  Set(0x210D, 0x77 | 0xFF << 16);
  Rect(0, 0, 40, 16, 0, kBlack);
  Set(0x2200, 0);
  Resolve(4, OUT(7), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 1, 8, 8);
  Set(0x2000, surface_b);
  Set(0x2001, 0x7C0);
  Set(0x2200, 1 | 2 << 8);
  Set(0x210D, 0x77 | 0xFF << 8);
  Rect(0, 0, 64, 64, 0, kWhite);
  Set(0x2200, 0);
  Resolve(0, OUT(8), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 1, 8, 0);
  // (h) C, 64 pixels wide without MSAA (sample (x, y) of pixel (x, y)),
  // colour at 0x7E0, the window's rows 0-32: white where the stencil is
  // 0x77 (its rows 0-32), the test loading it (8); then the window's
  // rows 0-64: the rows added load with their stencil at once (8), white
  // where it is 0xA5 (rows 32-64). Its colour and depth load twice each.
  Set(0x2000, 64);
  Set(0x2001, 0x7E0);
  Set(0x2082, 64 | 32 << 16);
  Set(0x2200, 1 | 2 << 8);
  Set(0x210D, 0x77 | 0xFF << 8);
  Rect(0, 0, 64, 32, 0, kWhite);
  Set(0x2082, 64 | 64 << 16);
  Set(0x210D, 0xA5 | 0xFF << 8);
  Rect(0, 32, 64, 64, 0, kWhite);
  Set(0x2200, 0);
  Resolve(0, OUT(9), 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 4, 16, 0);
  // (i) A's depth loads (its Z test on, Always) without its stencil;
  // white where the stencil test is on but Always and Keep (as if off: no
  // stencil bit); then red where the stencil is 0xA5 (A's rows 16-32),
  // which loads it (8).
  Set(0x2000, surface_a);
  Set(0x2001, 0x7D0);
  Set(0x2200, 1 | 2 | 7 << 4 | 7 << 8);
  Set(0x210D, 0xFFFF00);
  Rect(0, 0, 40, 64, 0, kWhite);
  Set(0x2200, 0);
  Resolve(0, SCRATCH, 6 << 7 | 2, 0);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 1, 0, 0);
  Set(0x2200, 1 | 2 << 8);
  Set(0x210D, 0xA5 | 0xFF << 8);
  Rect(0, 0, 40, 64, 0, kRed);
  Set(0x2200, 0);
  Resolve(0, OUT(10), 6 << 7 | 2, 0);
  Set(0x2000, 64);
  Set(0x2001, 0x200);
  Set(0x2002, 0x500);
  Kick();
  gq.used = 0;
  MarkStencil(1, 0, 0, 8, 0);
  volatile u32* t = At(mem);
#define T(k, x, y) t[0x1000 * (k) + Tiled64(x, y, 2) / 4]
  CHECK(T(0, 12, 12) == 0xFFFFFFFF && T(0, 28, 12) == 0 && T(0, 4, 4) == 0);
  CHECK(T(1, 8, 8) == 0xFF0000FF && T(1, 8, 40) == 0xFF00FF00 && T(1, 36, 40) == 0xFF0000FF);
  CHECK(T(2, 40, 4) == 0xFFFFFFFF && T(2, 40, 12) == 0);
  CHECK(T(3, 8, 20) == 0xFFFF0000 && T(3, 36, 20) == 0xFFFFFFFF);
  CHECK(T(4, 8, 12) == 0x800000A5 && T(4, 8, 40) == 0xFFFFFF3C);
  CHECK(T(5, 8, 8) == 0xFF0000FF && T(5, 8, 40) == 0xFF00FF00);
  CHECK(T(6, 16, 16) == 0xFFFF0000 && T(6, 56, 56) == 0xFFFF0000 && T(6, 40, 16) != 0xFFFF0000 &&
        T(6, 16, 40) != 0xFFFF0000);
  CHECK(T(7, 6, 6) == 0xFFFFFF77 && T(7, 6, 20) == 0x800000A5);
  CHECK(T(8, 12, 6) == 0xFFFFFFFF && T(8, 12, 20) != 0xFFFFFFFF);
  CHECK(T(9, 20, 16) == 0xFFFFFFFF && T(9, 20, 48) == 0xFFFFFFFF);
  CHECK(T(10, 6, 6) == 0xFFFFFFFF && T(10, 6, 40) == 0xFFFFFFFF && T(10, 20, 24) == 0xFF0000FF);
  MmFreePhysicalMemory(0, mem);
#undef T
#undef OUT
#undef SCRATCH
}

// Draws into colour and depth targets, resolved and read back: a vertex
// shader that fetches a position (x, y, z) and a colour and exports them,
// pixel shaders that export the colour interpolated or float constants.
// Screen-space vertices, as a driver's clears give them (no viewport
// scale or offset, XY and Z divided by W already, no clipping), pixel
// centres at integers.
static void Draws(volatile u32* rb, u32 w, u32 data, u32 pdata) {
  u32 mem = MmAllocatePhysicalMemoryEx(0, 0x80000, kPageReadWrite, 0, 0, 0x1000), pmem = MmGetPhysicalAddress(mem);
  gq.rb = rb;
  gq.w = w;
  gq.data = data;
  gq.pdata = pdata;
  gq.ib = At(mem);
  gq.pib = pmem;
  gq.pool = mem + 0x8000;
  gq.ppool = pmem + 0x8000;
  gq.ps = pmem + 0xF000;
  for (u32 k = 0x10000; k < 0x80000; k += 4) *At(mem + k) = 0xAAAAAAAA;
  // The shaders: the vertex shader in the packet, the pixel shaders in
  // memory.
  u32 vs[2][18], ps[5][9];
  for (u32 k = 0; k < 2; k++) {
    UcodeExec(vs[k], 1, 4 + k, 3);
    UcodeVfetch(vs[k] + 3, 1, 0, 57, 7, 0);  // r1: x, y, z (32_32_32_FLOAT)
    UcodeVfetch(vs[k] + 6, 2, 0, 38, 7, 3);  // r2: the colour (32_32_32_32_FLOAT)
    UcodeExport(vs[k] + 9, 62, 1, 1);        // the position
    UcodeExport(vs[k] + 12, 0, 2, 1);        // interpolator 0
    UcodeExport(vs[k] + 15, 63, 2, 1);       // point size, edge flag, kill (the second)
    for (u32 j = 0; j < 18; j++) *At(mem + 0xF400 + 0x80 * k + 4 * j) = vs[k][j];
  }
  gq.vs = pmem + 0xF400;
  UcodeExec(ps[0], 1, 1, 0);
  UcodeExport(ps[0] + 3, 0, 0, 1);  // colour 0: r0 (interpolator 0)
  UcodeExec(ps[1], 1, 1, 0);
  UcodeExport(ps[1] + 3, 0, 0, 0);  // colour 0: c0
  UcodeExec(ps[2], 1, 2, 0);
  UcodeExport(ps[2] + 3, 0, 0, 1);
  UcodeExport(ps[2] + 6, 1, 0, 0);  // colour 1: c0
  UcodeExec(ps[3], 1, 2, 0);
  UcodeExport(ps[3] + 3, 0, 0, 1);
  UcodeExport(ps[3] + 6, 61, 0, 0);  // depth: c0
  UcodeExec(ps[4], 1, 1, 0);
  UcodeExport(ps[4] + 3, 0, 1, 1);  // colour 0: r1
  for (u32 k = 0; k < 5; k++)
    for (u32 j = 0; j < 9; j++) *At(mem + 0xF000 + 0x40 * k + 4 * j) = ps[k][j];
  // The resolves' rectangles.
  const float corners[12] = {0, 0, 64, 0, 64, 64, 0, 0, 16, 0, 16, 16};
  for (u32 k = 0; k < 12; k++) *At(mem + 0xE000 + 4 * k) = FloatBits(corners[k]);
  gq.corners = pmem + 0xE000;
  gq.small = pmem + 0xE018;

  // A 64-pixel pitch without MSAA, colour target 0 at tile 0x300 (8888);
  // scissors open, no window offset; D3D's pixel centres; no blending,
  // all components written; no depth or stencil; no clipping.
  Put(Type0(0x2000, 2));
  Put(64);
  Put(0x300);
  Put(Type0(0x200E, 2));
  Put(0);
  Put(8192 | 8192 << 16);
  Put(Type0(0x2080, 3));
  Put(0);
  Put(0x80000000);
  Put(64 | 64 << 16);
  Put(Type0(0x2101, 2));    // VGT_MIN_VTX_INDX, VGT_INDX_OFFSET
  Put(0);
  Put(0);
  Set(0x2104, 0xF);         // RB_COLOR_MASK
  Set(0x2200, 0);           // RB_DEPTHCONTROL
  Set(0x2201, 0x00010001);  // RB_BLENDCONTROL0, 1
  Set(0x2209, 0x00010001);
  Set(0x2202, 0);           // RB_COLORCONTROL
  Set(0x2204, 1 << 16);     // PA_CL_CLIP_CNTL: no clipping
  Set(0x2205, 0);           // PA_SU_SC_MODE_CNTL
  Set(0x2206, 0x300);       // PA_CL_VTE_CNTL: XY and Z divided by W
  Set(0x2302, 0);
  Put(Type3(0x2B, 2 + 15));  // IM_LOAD_IMMEDIATE
  Put(0);
  Put(15);
  for (u32 k = 0; k < 15; k++) Put(vs[0][k]);

  // ---- 8_8_8_8, triangles and rectangles, store and load around resolves.
  // The target cleared to 0x11223344 (copied to the second texture, which
  // the last resolve writes again). Then an upper-right triangle in red 1
  // green 0.2 (0xFF0033FF as the EDRAM holds it); a triangle in green that
  // 16-bit indices 6-8 reach with an index offset of 3 (at 6-8 one they
  // would reach without it); a rectangle at (8.25, 16.25)-(16.25, 24.25)
  // (offset 3 too) in float constant 0, 0x33FF0000; resolved to the first
  // texture, the target cleared to 0x55667788; the rectangle again, to the
  // second.
  u32 dest1 = pmem + 0x10000, dest2 = pmem + 0x14000;
  Set(0x2208, 4);
  Set(0x231E, 0x11223344);
  Resolve(0x100, dest2, 6 << 7 | 2, 0);
  const float tri[6] = {0, 0, 32, 0, 32, 32}, rect[6] = {8.25f, 16.25f, 16.25f, 16.25f, 8.25f, 24.25f},
              decoy[6] = {40, 0, 60, 0, 60, 20}, green[6] = {40, 40, 60, 40, 60, 60};
  const float c_tri[4] = {1, 0.2f, 0, 1};
  u32 vb = Verts(tri, 3, 0, c_tri);
  Verts(rect, 3, 0, kBlack);
  Verts(decoy, 3, 0, kRed);
  Verts(green, 3, 0, kGreen);
  volatile uint16_t* indices = (volatile uint16_t*)(uintptr_t)(gq.pool + gq.used);
  u32 pindices = gq.ppool + gq.used;
  gq.used += 8;
  indices[0] = 6;
  indices[1] = 7;
  indices[2] = 8;
  PixelShader(0);
  SetF(0x4400, 0, 0, 1, 0.2f);  // pixel constant 0
  Fetch0(vb, 12);
  Put(Type3(0x36, 1));  // DRAW_INDX_2
  Put(3 << 16 | 2 << 6 | 4);
  Set(0x2102, 3);
  Put(Type3(0x22, 4));  // DRAW_INDX: query word, initiator, indices, count and swap
  Put(0);
  Put(3 << 16 | 0 << 6 | 4);
  Put(pindices);
  Put(3 | 1u << 30);
  PixelShader(1);
  u32 rectangle = gq.n;
  Put(Type3(0x36, 1));
  Put(3 << 16 | 2 << 6 | 8);
  Set(0x231E, 0x55667788);
  Resolve(0x100, dest1, 6 << 7 | 2, 0);
  Fetch0(vb, 12);
  Put(gq.ib[rectangle]);
  Put(gq.ib[rectangle + 1]);
  Resolve(0, dest2, 6 << 7 | 2, 0);
  Set(0x2102, 0);
  Kick();
  // Endian 2 without the red/blue swap: the words as the EDRAM holds
  // them. The triangle covers (28, 2), not (2, 28) (window Y down); the
  // rectangle, half a pixel on, x 9-16 and y 17-24, its fourth corner
  // (16, 24) too; the indexed triangle (58, 42), not (58, 2).
  volatile u32* t1 = At(mem + 0x10000);
  volatile u32* t2 = At(mem + 0x14000);
#define T1(x, y) t1[Tiled64(x, y, 2) / 4]
#define T2(x, y) t2[Tiled64(x, y, 2) / 4]
  CHECK(T1(28, 2) == 0xFF0033FF && T1(2, 28) == 0x11223344);
  CHECK(T1(9, 17) == 0x33FF0000 && T1(16, 24) == 0x33FF0000);
  CHECK(T1(8, 20) == 0x11223344 && T1(12, 16) == 0x11223344 && T1(17, 20) == 0x11223344 &&
        T1(12, 25) == 0x11223344);
  CHECK(T1(58, 42) == 0xFF00FF00 && T1(58, 2) == 0x11223344);
  // After the clear only the rectangle drawn again.
  CHECK(T2(9, 17) == 0x33FF0000 && T2(16, 24) == 0x33FF0000);
  CHECK(T2(28, 2) == 0x55667788 && T2(58, 42) == 0x55667788 && T2(8, 20) == 0x55667788);

  // ---- depth, stencil and render state: colour target 0 at 0x600, depth
  // (D24S8) at 0x500, both cleared (depth to 1, stencil 0).
  u32 dest_c = pmem + 0x18000, dest_d = pmem + 0x1C000, dest_d2 = pmem + 0x20000, dest_c1 = pmem + 0x24000;
  Set(0x2001, 0x600);
  Set(0x2002, 0x500);
  Set(0x2003, 0x610);  // colour target 1
  Set(0x231D, 0xFFFFFF00);
  Set(0x231E, 0);
  Resolve(0x100, dest_c, 6 << 7 | 2, 0);
  Resolve(0x100 | 1, dest_c1, 6 << 7 | 2, 0);
  Resolve(0x200 | 4, dest_d, 6 << 7 | 2, 0);
  PixelShader(0);
  // A (8-24): depth 0.5, less, written; stencil 0x5A replaced; red.
  Set(0x210D, 0xFFFF5A);  // RB_STENCILREFMASK: reference, read and write masks
  Set(0x2200, 1 | 2 | 4 | 1 << 4 | 7 << 8 | 2 << 14);
  Rect(8, 8, 24, 24, 0.5f, kRed);
  // A colour target at the depth target's tiles, rows 40-47 in 0x33: the
  // depth target is stored first, then loaded again (with these words).
  Set(0x2200, 0);
  Set(0x2001, 0x500);
  const float c_33[4] = {0.2f, 0.2f, 0.2f, 0.2f};
  Rect(0, 40, 64, 48, 0, c_33);
  Set(0x2001, 0x600);
  // B (0-32): where the stencil is 0x5A, depth 0.25 less, written; green.
  Set(0x2200, 1 | 2 | 4 | 1 << 4 | 2 << 8);
  Rect(0, 0, 32, 32, 0.25f, kGreen);
  // F (4-28): depth 0.375, less, written; blue where it passes.
  Set(0x2200, 1 | 2 | 4 | 1 << 4 | 7 << 8);
  Rect(4, 4, 28, 28, 0.375f, kBlue);
  // Depth only (EDRAM mode 5): depth 0.125 at (36-44, 4-12), no colour.
  Set(0x2200, 2 | 4 | 7 << 4);
  Set(0x2208, 5);
  Rect(36, 4, 44, 12, 0.125f, kWhite);
  Set(0x2208, 4);
  // The pixel shader's depth, 0.625 (float constant 0), at (48-56, 4-12).
  PixelShader(3);
  SetF(0x4400, 0.625f, 0, 0, 0);
  Rect(48, 4, 56, 12, 0, kWhite);
  PixelShader(0);
  Set(0x2200, 0);
  // Blending: the constant colour plus the target (0) at (36-44, 16-24),
  // 0.25 0.75 0 1 (0xFF00BF40); the constant's alpha 0.25 times the
  // colour at (48-56, 16-24) (0x40404040).
  Set(0x2201, 12 | 1 << 8 | 12 << 16 | 1 << 24);
  SetF(0x2105, 0.25f, 0.75f, 0, 1);  // RB_BLEND_RED-ALPHA
  Rect(36, 16, 44, 24, 0, kWhite);
  Set(0x2201, 14 | 14 << 16);
  SetF(0x2105, 0, 0, 0, 0.25f);
  Rect(48, 16, 56, 24, 0, kWhite);
  Set(0x2201, 0x00010001);
  // The alpha test, greater than 0.6: alpha 0.5 at (36-44, 28-36) fails,
  // 0.8 at (48-56, 28-36) passes (0xCCFFFFFF).
  Set(0x2202, 8 | 4);
  Set(0x210E, FloatBits(0.6f));  // RB_ALPHA_REF
  const float c_half[4] = {1, 1, 1, 0.5f}, c_08[4] = {1, 1, 1, 0.8f};
  Rect(36, 28, 44, 36, 0, c_half);
  Rect(48, 28, 56, 36, 0, c_08);
  Set(0x2202, 0);
  // Culling: triangles clockwise as the window shows them (and one
  // counter-clockwise) at x 0, 16, 32, 48, y 40-56. Back faces culled,
  // front counter-clockwise: the clockwise one goes, the other stays;
  // front clockwise: it stays; front faces culled, front counter-
  // clockwise: it stays.
  const float cw0[6] = {0, 40, 16, 40, 16, 56}, ccw1[6] = {16, 40, 32, 56, 32, 40}, cw2[6] = {32, 40, 48, 40, 48, 56},
              cw3[6] = {48, 40, 64, 40, 64, 56};
  Set(0x2205, 2);
  Tri(cw0, 0, kWhite);
  Tri(ccw1, 0, kWhite);
  Set(0x2205, 2 | 4);
  Tri(cw2, 0, kWhite);
  Set(0x2205, 1);
  Tri(cw3, 0, kWhite);
  Set(0x2205, 0);
  // The colour mask: red and blue at (0-8, 56-64).
  Set(0x2104, 5);
  Rect(0, 56, 8, 64, 0, kWhite);
  Set(0x2104, 0xF);
  // D24FS8 at 0x520: depth 0.75 at (16-24, 56-64); a colour target at its
  // tiles (rows 0-7) has it stored; then, loaded again, a white rectangle
  // where the depth equals 0.75.
  Set(0x231D, 0);
  Set(0x2002, 0x520 | 1 << 16);
  Resolve(0x200 | 4, dest_d2, 6 << 7 | 2, 0);
  Set(0x2200, 2 | 4 | 7 << 4);
  Rect(16, 56, 24, 64, 0.75f, kBlack);
  // And the pixel shader's depth 0.75 + 5 ulp at (32-40, 56-64): its 20e4
  // rounded to the nearest even (0xE80001).
  const union {
    u32 u;
    float f;
  } z5 = {0x3F400005};
  PixelShader(3);
  SetF(0x4400, z5.f, 0, 0, 0);
  Rect(32, 56, 40, 64, 0, kBlack);
  PixelShader(0);
  Set(0x2200, 0);
  Set(0x2001, 0x520);
  Rect(0, 0, 8, 8, 0, kWhite);
  Set(0x2001, 0x600);
  Set(0x2200, 2 | 2 << 4);
  Rect(16, 56, 24, 64, 0.75f, kWhite);
  Set(0x2200, 0);
  // Two colour targets: the interpolated red to 0, float constant 0 (blue)
  // to 1, at (24-32, 56-64), both written (RB_COLOR_MASK has four bits a
  // target).
  PixelShader(2);
  SetF(0x4400, 0, 0, 1, 1);
  Set(0x2104, 0xFF);
  Rect(24, 56, 32, 64, 0, kRed);
  Set(0x2104, 0xF);
  PixelShader(0);
  Resolve(0, dest_c, 6 << 7 | 2, 0);
  Resolve(1, dest_c1, 6 << 7 | 2, 0);
  Resolve(4, dest_d2, 6 << 7 | 2, 0);
  Set(0x2002, 0x500);
  Resolve(4, dest_d, 6 << 7 | 2, 0);
  Kick();
  volatile u32* tc = At(mem + 0x18000);
  volatile u32* td = At(mem + 0x1C000);
  volatile u32* td2 = At(mem + 0x20000);
  volatile u32* tc1 = At(mem + 0x24000);
#define TC(x, y) tc[Tiled64(x, y, 2) / 4]
#define TD(x, y) td[Tiled64(x, y, 2) / 4]
  // B over A where the (loaded) stencil is 0x5A; F around it, not inside
  // (0.375 is not less than 0.25); nothing outside.
  CHECK(TC(16, 16) == 0xFF00FF00 && TC(6, 6) == 0xFFFF0000 && TC(30, 30) == 0);
  // 24-bit depth over stencil: 0.25 (0x400000, rounded to the nearest
  // even), 0.375 (0x600000), 1, depth only 0.125 (0x200000), the pixel
  // shader's 0.625 (0x9FFFFF).
  CHECK(TD(16, 16) == 0x4000005A && TD(6, 6) == 0x60000000 && TD(30, 30) == 0xFFFFFF00);
  CHECK(TD(40, 8) == 0x20000000 && TC(40, 8) == 0);
  CHECK(TD(52, 8) == 0x9FFFFF00 && TC(52, 8) == 0xFFFFFFFF);
  // The colour target drawn at the depth's tiles: its sample 16 is depth
  // sample 56 (a depth tile's halves are swapped).
  CHECK(TD(56, 44) == 0x33333333 && TD(16, 20) == 0x4000005A);
  CHECK(TC(40, 20) == 0xFF00BF40 && TC(52, 20) == 0x40404040);
  CHECK(TC(40, 32) == 0 && TC(52, 32) == 0xCCFFFFFF);
  CHECK(TC(12, 43) == 0 && TC(28, 43) == 0xFFFFFFFF && TC(44, 43) == 0xFFFFFFFF && TC(60, 43) == 0xFFFFFFFF);
  CHECK(TC(4, 60) == 0x00FF00FF);
  // D24FS8: 0.75 as 20e4 (0xE80000), equal after its load; 0.75 + 5 ulp.
  CHECK(td2[Tiled64(20, 60, 2) / 4] == 0xE8000000 && TC(20, 60) == 0xFFFFFFFF);
  CHECK(td2[Tiled64(36, 60, 2) / 4] == 0xE8000100);
  CHECK(TC(28, 60) == 0xFF0000FF && tc1[Tiled64(28, 60, 2) / 4] == 0xFFFF0000);

  // ---- each colour format at 0x400, its words resolved as they are.
  u32 n_formats = sizeof kFormats / sizeof kFormats[0];
  PixelShader(1);
  for (u32 k = 0; k < n_formats; k++) {
    u32 format = kFormats[k].info & 0xF, wide = format == 5 || format == 7 || format == 15;
    Set(0x2001, 0x400 | kFormats[k].info << 16);
    SetF(0x4400, kFormats[k].c[0], kFormats[k].c[1], kFormats[k].c[2], kFormats[k].c[3]);
    Rect(0, 0, 16, 16, 0, kWhite);
    Set(0x2001, 0x400 | (wide ? 15 : 14) << 16);
    Resolve(0, pmem + 0x28000 + 0x2000 * k, (wide ? 37 : 36) << 7 | 2, 1);
  }
  Set(0x2001, 0x600);
  PixelShader(0);
  Kick();
  for (u32 k = 0; k < n_formats; k++) {
    u32 format = kFormats[k].info & 0xF, wide = format == 5 || format == 7 || format == 15;
    u32 at = mem + 0x28000 + 0x2000 * k + Tiled64(4, 4, wide ? 3 : 2);
    if (*At(at) != kFormats[k].w0 || (wide && *At(at + 4) != kFormats[k].w1))
      DbgPrint("FAIL line %d: colour format %u (row %u): %08x %08x\n", __LINE__, kFormats[k].info, k, *At(at),
               *At(at + 4));
  }

  // ---- MSAA: 4x at 0x700 and 2x at 0x720, cleared, a white rectangle from
  // (9, 9), its edges half a pixel on through samples: of pixel (9, 12)
  // the right ones covered, of (12, 9) the lower. Resolved: samples 0 and
  // 2 (4x) or 1 (2x), and the average.
  for (u32 m = 0; m < 2; m++) {
    u32 base = pmem + 0x48000 + 0xC000 * m;
    Set(0x2000, 64 | (m ? 1 : 2) << 16);
    Set(0x2001, m ? 0x720 : 0x700);
    Resolve(0x100, base, 6 << 7 | 2, 0);
    Rect(9, 9, 20, 20, 0, kWhite);
    Resolve(0, base, 6 << 7 | 2, 0);
    Resolve((m ? 1 : 2) << 4, base + 0x4000, 6 << 7 | 2, 0);
    Resolve((m ? 4 : 6) << 4, base + 0x8000, 6 << 7 | 2, 0);
  }
  Set(0x2000, 64);
  Set(0x2001, 0x600);
  Kick();
  volatile u32* m4 = At(mem + 0x48000);
  volatile u32* m2 = At(mem + 0x54000);
#define M(t, k, x, y) t[0x1000 * (k) + Tiled64(x, y, 2) / 4]
  CHECK(M(m4, 0, 9, 12) == 0 && M(m4, 1, 9, 12) == 0xFFFFFFFF && M(m4, 2, 9, 12) == 0x80808080);
  CHECK(M(m4, 0, 12, 9) == 0 && M(m4, 1, 12, 9) == 0 && M(m4, 2, 12, 9) == 0x80808080);
  CHECK(M(m4, 0, 12, 12) == 0xFFFFFFFF && M(m4, 2, 12, 12) == 0xFFFFFFFF && M(m4, 0, 8, 12) == 0);
  CHECK(M(m2, 0, 12, 9) == 0 && M(m2, 1, 12, 9) == 0xFFFFFFFF && M(m2, 2, 12, 9) == 0x80808080);
  CHECK(M(m2, 0, 10, 12) == 0xFFFFFFFF && M(m2, 1, 10, 12) == 0xFFFFFFFF && M(m2, 0, 8, 12) == 0);
#undef T1
#undef T2
#undef TC
#undef TD
#undef M
  Primitives();
  Textures();
  InPlace();
  StencilLoads();
}

// Waits up to a second for *p == v.
static void Gpu(void) {
  CHECK(*Reg(0x1961) == (1280u << 16 | 720) && *Reg(0x1951) == 1);
  // Vertical blanks reach the callback as DPCs.
  VdSetGraphicsInterruptCallback((void*)interrupt_entry, 0x5678);
  int64_t hundred_ms = -1000000;
  KeDelayExecutionThread(1, 0, &hundred_ms);
  CHECK(interrupts[0] >= 3 && interrupt_user == 0x5678 && interrupt_irql == 2);

  // A 4 KiB ring buffer, then the memory the GPU writes, by physical
  // address (the GPU's addresses) through another view.
  u32 ring = MmAllocatePhysicalMemoryEx(0, 0x3000, kPageReadWrite, 0, 0, 0x1000), pa = MmGetPhysicalAddress(ring);
  u32 data = ring + 0x1000, pdata = pa + 0x1000;
  CHECK(VdInitializeRingBuffer(pa, 9) == 0);
  VdEnableRingBufferRPtrWriteBack(pdata, 6);
  volatile u32* rb = At(ring);
  u32 w = 0;
  // Scratch register 0, written back to memory.
  rb[w++] = Type0(0x1DC, 2);
  rb[w++] = 1;
  rb[w++] = pdata + 4;
  rb[w++] = Type0(0x578, 1);
  rb[w++] = 0x1234;
  // Words to memory: the GPU is little-endian, 2 swaps bytes in words.
  rb[w++] = Type3(0x3D, 2);
  rb[w++] = (pdata + 0x40) | 2;
  rb[w++] = 0xCAFEBABE;
  rb[w++] = Type3(0x3D, 2);
  rb[w++] = pdata + 0x44;
  rb[w++] = 0xCAFEBABE;
  // An indirect buffer with a fence.
  volatile u32* ib = At(data + 0x100);
  ib[0] = Type3(0x58, 3);
  ib[1] = 0;
  ib[2] = (pdata + 0x60) | 2;
  ib[3] = 7;
  rb[w++] = Type3(0x3F, 2);
  rb[w++] = pdata + 0x100;
  rb[w++] = 4;
  // An interrupt for processor 0.
  rb[w++] = Type3(0x54, 1);
  rb[w++] = 1;
  // A wait for the title's flag, then a fence.
  rb[w++] = Type3(0x3C, 5);
  rb[w++] = 0x13;
  rb[w++] = (pdata + 0x80) | 2;
  rb[w++] = 1;
  rb[w++] = 0xFFFFFFFF;
  rb[w++] = 0x100;
  rb[w++] = Type3(0x58, 3);
  rb[w++] = 0;
  rb[w++] = (pdata + 0x64) | 2;
  rb[w++] = 9;
  // Occlusion queries: a begin's counts zeroed, an end's (D3D's mark in
  // a z fail word) samples passed, little-endian.
  volatile u32* query = At(data + 0xC0);
  for (u32 k = 0; k < 16; k++) query[k] = 0x11111111;
  query[3] = 0xFFFFFEED;
  rb[w++] = Type0(0x2325, 1);
  rb[w++] = pdata + 0xC0;
  rb[w++] = Type3(0x5B, 1);
  rb[w++] = 0x15;
  rb[w++] = Type0(0x2325, 1);
  rb[w++] = pdata + 0xE0;
  rb[w++] = Type3(0x5B, 1);
  rb[w++] = 0x15;
  // The swap: VdSwap fills 64 words.
  // Its front buffer: a linear 32x8 k_8_8_8_8 texture (format 6), the
  // swizzle XYZW.
  u32 fb = ring + 0x2000, fetch[6] = {1 << 22, fb | 6, 31 | 7 << 13, (0 | 1 << 3 | 2 << 6 | 3 << 9) << 1, 0, 0},
      front = fb, format = 6, color_space = 0, width = 1280, height = 720;
  u32 swap = w;
  VdSwap10(&rb[w], fetch, 0, 0, 0, &front, &format, &color_space, &width, &height);
  w += 64;
  CHECK(rb[swap] == 0x00054800 && rb[swap + 2] == (pa + 0x2000 | 6) && rb[swap + 7] == 0xC0036400 &&
        rb[swap + 8] == 0x53574150 && rb[swap + 9] == pa + 0x2000 && rb[swap + 63] == 0x80000000);
  *Reg(0x1C5) = w;  // CP_RB_WPTR

  CHECK(WaitFor(At(data + 0x60), 7));
  CHECK(*At(data + 4) == 0x1234 && *At(data + 0x40) == 0xCAFEBABE && *At(data + 0x44) == 0xBEBAFECA);
  CHECK(WaitFor(&interrupts[1], 1));
  // Held at the wait until the flag is set.
  CHECK(*At(data + 0x64) == 0);
  *At(data + 0x80) = 1;
  CHECK(WaitFor(At(data + 0x64), 9) && WaitFor(At(data), w) && *Reg(0x1C4) == w);
  CHECK(query[0] == 0xE8030000 && query[4] == 0xE8030000 && query[1] == 0 && query[3] == 0 && query[7] == 0);
  CHECK(query[8] == 0 && query[12] == 0 && query[15] == 0);
  w = Resolves(rb, w, data, pdata);
  Draws(rb, w, data, pdata);
}

// A thread that waits on an event and counts itself through.
static volatile u32 pulsed;
static u32 PulseWaiter(void* arg) {
  NtWaitForSingleObjectEx((u32)(uintptr_t)arg, 1, 0, 0);
  __sync_fetch_and_add(&pulsed, 1);
  return 0;
}

// A set satisfies the waits it completes when it happens: every thread
// waiting on a notification event, though a reset follows at once; one
// per set of a synchronization event.
static void Handoff(void) {
  int64_t fifty_ms = -500000;
  for (u32 type = 0; type < 2; type++) {
    u32 ev[4] = {type << 24, 0, 0, 0}, threads[2], id;
    ev[2] = ev[3] = (u32)(uintptr_t)&ev[2];
    handed[0] = handed[1] = 0;
    for (u32 k = 0; k < 2; k++)
      CHECK(ExCreateThread(&threads[k], 0, &id, (void*)thread_startup, (void*)EventWaiter, ev, 0) == 0);
    KeDelayExecutionThread(1, 0, &fifty_ms);
    KeSetEvent(ev, 1, 0);
    if (type) KeSetEvent(ev, 1, 0);
    else KeResetEvent(ev);
    for (u32 k = 0; k < 2; k++) {
      NtWaitForSingleObjectEx(threads[k], 1, 0, 0);
      NtClose(threads[k]);
    }
    CHECK(handed[0] == 2 && handed[1] == 0 && ev[1] == 0);
  }
}

// A thread that ends owning a mutant abandons it.
static u32 mutant;
static u32 MutantOwner(void* arg) {
  (void)arg;
  int64_t zero = 0, fifty_ms = -500000;
  u32 status = NtWaitForSingleObjectEx(mutant, 1, 0, &zero);
  KeDelayExecutionThread(1, 0, &fifty_ms);
  return status;
}

static void Owners(void) {
  int64_t zero = 0, ten_ms = -100000, second = -10000000;
  u32 h = 0, e = 0, previous = 9;
  // The next wait to take an abandoned mutant gets STATUS_ABANDONED (plus
  // its index) and clears the flag: a waiter blocked on it, and one after.
  CHECK(NtCreateMutant(&mutant, 0, 0) == 0 && NtCreateEvent(&e, 0, 0, 0) == 0);
  CHECK(ExCreateThread(&h, 0, 0, (void*)thread_startup, (void*)MutantOwner, 0, 0) == 0);
  KeDelayExecutionThread(1, 0, &ten_ms);
  u32 both[2] = {e, mutant};
  CHECK(NtWaitForMultipleObjectsEx(2, both, 1, 1, 0, &second) == 0x81);
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0);
  NtClose(h);
  CHECK(NtReleaseMutant(mutant, &previous) == 0 && previous == 0);
  CHECK(ExCreateThread(&h, 0, 0, (void*)thread_startup, (void*)MutantOwner, 0, 0) == 0);
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0);
  NtClose(h);
  CHECK(NtWaitForSingleObjectEx(mutant, 1, 0, &zero) == 0x80);
  CHECK(NtReleaseMutant(mutant, &previous) == 0 && previous == 0);
  CHECK(NtWaitForSingleObjectEx(mutant, 1, 0, &zero) == 0);
  CHECK(NtReleaseMutant(mutant, &previous) == 0);
  NtClose(mutant);
  NtClose(e);
}

// A wait on a handle closed meanwhile goes on on its object.
static u32 HandleWaiter(void* handle) {
  int64_t timeout = -2000000;  // 200 ms
  return NtWaitForSingleObjectEx((u32)(uintptr_t)handle, 1, 0, &timeout);
}

// The event a wait is on outlives its handle: an event created after the
// close is another one, and its set does not end the wait.
static void Closes(void) {
  int64_t ten_ms = -100000;
  u32 h = 0, e = 0, obj = 0;
  CHECK(NtCreateEvent(&e, 0, 0, 0) == 0);
  CHECK(ExCreateThread(&h, 0, 0, (void*)thread_startup, (void*)HandleWaiter, (void*)(uintptr_t)e, 0) == 0);
  KeDelayExecutionThread(1, 0, &ten_ms);
  u32 other = 0;
  CHECK(NtClose(e) == 0 && NtCreateEvent(&other, 0, 0, 0) == 0 && NtSetEvent(other, 0) == 0);
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0);
  CHECK(ObReferenceObjectByHandle(h, __imp_ExThreadObjectType, &obj) == 0 && *At(obj + 0x140) == kStatusTimeout);
  ObDereferenceObject(obj);
  NtClose(h);
  NtClose(other);
}

// PulseEvent lets the threads waiting through, all of them for a
// notification event, one for a synchronization event, and leaves the
// event unsignalled.
static void Pulses(void) {
  int64_t fifty_ms = -500000, zero = 0;
  for (u32 type = 0; type < 2; type++) {
    u32 e = 0, threads[2], id, previous = 9;
    pulsed = 0;
    CHECK(NtCreateEvent(&e, 0, type, 0) == 0);
    for (u32 k = 0; k < 2; k++)
      CHECK(ExCreateThread(&threads[k], 0, &id, (void*)thread_startup, (void*)PulseWaiter, (void*)(uintptr_t)e, 0) == 0);
    KeDelayExecutionThread(1, 0, &fifty_ms);
    CHECK(NtPulseEvent(e, &previous) == 0 && previous == 0);
    CHECK(WaitFor(&pulsed, type ? 1 : 2));
    KeDelayExecutionThread(1, 0, &fifty_ms);
    CHECK(pulsed == (type ? 1 : 2) && NtWaitForSingleObjectEx(e, 1, 0, &zero) == 0x102);
    if (type) {
      CHECK(NtSetEvent(e, 0) == 0 && WaitFor(&pulsed, 2));
    }
    for (u32 k = 0; k < 2; k++) {
      NtWaitForSingleObjectEx(threads[k], 1, 0, 0);
      NtClose(threads[k]);
    }
    NtClose(e);
  }
}

// ---- strings, statuses, the FPU flag ------------------------------------------
static void Conversions(void) {
  uint16_t wide[8];
  u32 written = 0;
  CHECK(RtlMultiByteToUnicodeN(wide, sizeof wide, &written, "abc\xE9", 4) == 0 && written == 8 && wide[0] == 'a' &&
        wide[3] == 0xE9);
  CHECK(RtlMultiByteToUnicodeN(wide, 4, &written, "abcdef", 6) == 0 && written == 4);
  static const uint16_t hi[3] = {'h', 'i', 0x263A};
  char narrow[8];
  CHECK(RtlUnicodeToMultiByteN(narrow, sizeof narrow, &written, hi, 6) == 0 && written == 3 &&
        Memeq(narrow, "hi?", 3));
  struct Ansi u = {6, 6, (u32)(uintptr_t)hi}, a = {0, 0, 0};
  CHECK(RtlUnicodeStringToAnsiString(&a, &u, 1) == 0 && a.length == 3 && Memeq((void*)(uintptr_t)a.buffer, "hi?", 4));
  RtlFreeAnsiString(&a);
  char small[2];
  struct Ansi s = {0, 2, (u32)(uintptr_t)small};
  CHECK(RtlUnicodeStringToAnsiString(&s, &u, 0) == 0x80000005 && s.length == 1 && Memeq(small, "h", 2));
  CHECK(RtlNtStatusToDosError(0) == 0 && RtlNtStatusToDosError(0xC0000034) == 2 &&
        RtlNtStatusToDosError(0x80070005) == 5 && RtlNtStatusToDosError(0xC0000135) == 126 &&
        RtlNtStatusToDosError(0xC0000240) == 1235 && RtlNtStatusToDosError(0xC00000F0) == 87);
  uint64_t time = 0;
  uint16_t f[8];
  RtlTimeToTimeFields(&time, f);
  CHECK(f[0] == 1601 && f[1] == 1 && f[2] == 1 && f[3] == 0 && f[6] == 0 && f[7] == 1);  // a Monday
  time = 0x1da6b158f2a0f87ull;  // 2024-02-29 13:45:30.1234567, a Thursday
  RtlTimeToTimeFields(&time, f);
  CHECK(f[0] == 2024 && f[1] == 2 && f[2] == 29 && f[3] == 13 && f[4] == 45 && f[5] == 30 && f[6] == 123 &&
        f[7] == 4);
  time = 6 * 864000000000ull;  // 1601-01-07, a Sunday
  RtlTimeToTimeFields(&time, f);
  CHECK(f[2] == 7 && f[7] == 0);
  // And back, the weekday ignored; no such date is FALSE.
  uint16_t fields[8] = {2006, 11, 7, 12, 34, 56, 789, 6};
  CHECK(RtlTimeFieldsToTime(fields, &time) == 1 && time == 0x1c7026921f53c50ull);
  fields[1] = 2, fields[2] = 29;
  CHECK(RtlTimeFieldsToTime(fields, &time) == 0 && time == 0x1c7026921f53c50ull);
  uint16_t first[8] = {1601, 1, 1, 0, 0, 0, 0, 0};
  CHECK(RtlTimeFieldsToTime(first, &time) == 1 && time == 0);
  volatile uint8_t* fpu = (volatile uint8_t*)(uintptr_t)(*At((u32)pcr() + 0x100) + 0x71);
  KeEnableFpuExceptions(1);
  CHECK(*fpu == 1);
  KeEnableFpuExceptions(0);
  CHECK(*fpu == 0);
}

// ---- audio ---------------------------------------------------------------------
// The render driver's callback (start.S's audio_entry), given a pointer to
// the client's argument.
static volatile u32 audio_calls, audio_arg;
void AudioCallback(u32 arg) {
  audio_calls++;
  audio_arg = *At(arg);
}

static void Audio(void) {
  u32 registration[2] = {(u32)(uintptr_t)audio_entry, 0x1357}, driver = 0;
  CHECK(XAudioRegisterRenderDriverClient(registration, &driver) == 0 && (driver >> 16) == 0x4155);
  // A frame every 256 samples at 48 kHz: about 19 in 100 ms.
  int64_t hundred_ms = -1000000;
  KeDelayExecutionThread(1, 0, &hundred_ms);
  u32 calls = audio_calls;
  CHECK(calls >= 10 && calls <= 30 && audio_arg == 0x1357);
  static float frame[6 * 256];
  CHECK(XAudioSubmitRenderDriverFrame(driver, frame) == 0);
  u32 mask = 1, volume = 0;
  CHECK(XAudioGetVoiceCategoryVolumeChangeMask(driver, &mask) == 0 && mask == 0);
  CHECK(XAudioGetVoiceCategoryVolume(0, &volume) == 0 && volume == 0x3F800000);
  CHECK(XAudioUnregisterRenderDriverClient(driver) == 0);
  // At most a callback already under way after that.
  calls = audio_calls;
  KeDelayExecutionThread(1, 0, &hundred_ms);
  CHECK(audio_calls - calls <= 1);
}

// ---- the XMA decoder -------------------------------------------------------------
// Its registers are little-endian: the array's physical address at 0x600,
// a bit per context in the clear registers from 0x6A0.
static volatile u32* XmaReg(u32 r) { return At(0x7FEA0000 + 4 * r); }

static void Xma(void) {
  u32 array = __builtin_bswap32(*XmaReg(0x600)), a = 0, b = 0;
  CHECK(XMACreateContext(&a) == 0 && XMACreateContext(&b) == 0 && a && b && a != b && !(a & 63) && !(b & 63));
  u32 index = (MmGetPhysicalAddress(a) - array) >> 6;
  CHECK(index < 320 && ((MmGetPhysicalAddress(b) - array) >> 6) < 320 && At(a)[0] == 0 && At(a)[15] == 0);
  // A clear: no buffer valid, the read offset past the packet header; the
  // packet count stays.
  At(a)[0] = 0x00300FFF;
  At(a)[1] = 0x80000000;
  At(a)[2] = 0x12345;
  *XmaReg(0x6A0 + (index >> 5)) = __builtin_bswap32(1u << (index & 31));
  CHECK(WaitFor(&At(a)[2], 32) && At(a)[0] == 0x00000FFF && At(a)[1] == 0);
  // A lock, with nothing decoding, is taken and leaves the context alone.
  At(a)[0] = 0x00300FFF;
  *XmaReg(0x690 + (index >> 5)) = __builtin_bswap32(1u << (index & 31));
  CHECK(WaitFor(XmaReg(0x690 + (index >> 5)), 0) && At(a)[0] == 0x00300FFF && At(a)[2] == 32);
  // Released contexts are zeroed and handed out again.
  CHECK(XMAReleaseContext(a) == 0);
  u32 again = 0;
  CHECK(XMACreateContext(&again) == 0 && again == a && At(a)[2] == 0);
  // All 320 in use: STATUS_NO_MEMORY.
  static u32 contexts[320];
  u32 n = 0;
  while (n < 320 && XMACreateContext(&contexts[n]) == 0) n++;
  u32 none = 1;
  CHECK(n == 318 && XMACreateContext(&none) == 0xC0000017 && none == 0);
  for (u32 k = 0; k < n; k++) XMAReleaseContext(contexts[k]);
  CHECK(XMAReleaseContext(a) == 0 && XMAReleaseContext(b) == 0);
}

// Decoding. The frames are mono and silent (no coefficients), of a given
// length in bits, the rest fill bits of the subframe's extended header:
// WMA Pro's frame layout, as ffmpeg's decoder reads it. Bits go into a
// buffer of 2 KiB packets, past each packet's 32-bit header.
static void XmaPut(uint8_t* buffer, u32* at, u32 n, u32 value) {
  while (n--) {
    if (*at % 16384 == 0) *at += 32;
    uint8_t bit = 0x80 >> (*at & 7);
    buffer[*at >> 3] = (value >> n & 1) ? buffer[*at >> 3] | bit : buffer[*at >> 3] & ~bit;
    ++*at;
  }
}

static void XmaFrame(uint8_t* buffer, u32* at, u32 length, u32 more) {
  u32 fill = length - 49;
  XmaPut(buffer, at, 15, length);
  XmaPut(buffer, at, 2, 2);         // one subframe layout for all channels; one of 512 samples
  XmaPut(buffer, at, 9, 0);         // dynamic range gain; no trim
  XmaPut(buffer, at, 7, 0x4C);      // an extended header, its fill length in 12 bits
  XmaPut(buffer, at, 12, fill - 1);
  for (u32 k = 0; k < fill; k++) XmaPut(buffer, at, 1, 0);
  XmaPut(buffer, at, 3, 0);         // reserved; no coefficients; the bit before the last
  XmaPut(buffer, at, 1, more);      // another frame follows in the packet
}

// An XMA2 packet header: frames that begin in it, the first one's offset
// past the header, XMA2 metadata (1), no packets of other streams after it.
static void XmaHeader(uint8_t* packet, u32 frames, u32 first) {
  packet[0] = (uint8_t)(frames << 2 | first >> 13);
  packet[1] = (uint8_t)(first >> 5);
  packet[2] = (uint8_t)((first & 31) << 3 | 1);
  packet[3] = 0;
}

// Kicks a context and waits until the decoder fills the output ring (its
// valid bit cleared) or writes some of it (the write offset moved). The
// blocks written.
static u32 XmaKick(volatile u32* context, u32 index, u32 blocks) {
  u32 write = context[0] >> 27;
  *XmaReg(0x650 + (index >> 5)) = __builtin_bswap32(1u << (index & 31));
  int64_t ms = -10000;
  for (int k = 0; k < 1000 && context[1] >> 31 && context[0] >> 27 == write; k++) KeDelayExecutionThread(1, 0, &ms);
  return context[1] >> 31 ? ((context[0] >> 27) + blocks - write) % blocks : blocks;
}

static int XmaSilent(const uint8_t* p, u32 bytes) {
  for (u32 k = 0; k < bytes; k++)
    if (p[k]) return 0;
  return 1;
}

static void XmaDecode(void) {
  u32 array = __builtin_bswap32(*XmaReg(0x600)), va = 0;
  CHECK(XMACreateContext(&va) == 0);
  u32 index = (MmGetPhysicalAddress(va) - array) >> 6;
  volatile u32* c = At(va);
  // Two packets of eight 4000-bit frames: four whole in the first, the
  // fifth run on into the second (whose first frame begins after its
  // 3648 bits; the fifth's last bit, 0, does not end the stream there),
  // the last followed by padding.
  u32 memory = MmAllocatePhysicalMemoryEx(0, 0x2000, kPageReadWrite, 0, 0, 0x1000);
  uint8_t* input = (uint8_t*)(uintptr_t)memory;
  uint8_t* output = input + 0x1000;
  for (u32 k = 0; k < 4096; k++) input[k] = 0xFF;
  XmaHeader(input, 5, 0);
  XmaHeader(input + 2048, 3, 3648);
  u32 at = 32;
  for (u32 f = 0; f < 8; f++) XmaFrame(input, &at, 4000, f < 7 && f != 4);
  u32 pa = MmGetPhysicalAddress(memory);

  // Mono at 48 kHz into a ring of 8 blocks, 2 at a time. The first frame
  // gives no samples; a frame is 4 blocks: two frames fill the ring.
  // The title empties it and sets it valid again.
  *XmaReg(0x6A0 + (index >> 5)) = __builtin_bswap32(1u << (index & 31));
  CHECK(WaitFor(&c[2], 32));
  c[5] = pa;
  c[7] = pa + 0x1000;
  c[1] = 1u << 31 | 3u << 27 | 2u << 20;
  c[0] = 8u << 22 | 1u << 20 | 2;
  u32 total = 0;
  for (int round = 0; round < 4; round++) {
    for (u32 k = 0; k < 2048; k++) output[k] = 0x5A;
    u32 blocks = XmaKick(c, index, 8);
    total += blocks;
    CHECK(XmaSilent(output, blocks * 256) && output[blocks * 256 % 2048] == (blocks == 8 ? 0 : 0x5A));
    c[1] |= 1u << 31;
  }
  // Seven frames out; input buffer 0 left (its valid bit cleared, buffer 1
  // current), the read offset past the next packet's header.
  CHECK(total == 28 && (c[0] & 0xF8300000) == 0x20000000 && c[2] == 32 && c[4] >> 31 == 1);

  // A loop, once, from the second frame (bit 4032) to the third (8032),
  // whose subframes 0 and 1 end it, and which starts again past the second
  // frame's subframe 0, one block at a time: 4 + 2 + 3 + 4 + 5 * 4 blocks.
  c[2] = 0;
  *XmaReg(0x6A0 + (index >> 5)) = __builtin_bswap32(1u << (index & 31));
  CHECK(WaitFor(&c[2], 32));
  c[3] = 4032;
  c[4] = 8032;
  c[1] = 1u << 31 | 3u << 27 | 1u << 20 | 1u << 17 | 1u << 14;
  c[0] = 8u << 22 | 1u << 20 | 1u << 12 | 2;
  total = 0;
  for (int round = 0; round < 5; round++) {
    total += XmaKick(c, index, 8);
    c[1] |= 1u << 31;
  }
  CHECK(total == 33 && (c[0] >> 12 & 0xFF) == 0 && !(c[0] >> 20 & 1));

  // Both buffers valid, a packet each, no frame beginning in either: the
  // scan for a frame goes through both, consumes them and stops, back at
  // buffer 0 past its header.
  c[2] = 0;
  *XmaReg(0x6A0 + (index >> 5)) = __builtin_bswap32(1u << (index & 31));
  CHECK(WaitFor(&c[2], 32));
  XmaHeader(input, 0, 0x7FFF);
  XmaHeader(input + 2048, 0, 0x7FFF);
  c[3] = c[4] = 0;
  c[6] = pa + 2048;
  c[1] = 1u << 31 | 3u << 27 | 1u << 20 | 1;
  c[0] = 8u << 22 | 3u << 20 | 1;
  *XmaReg(0x650 + (index >> 5)) = __builtin_bswap32(1u << (index & 31));
  int64_t ms = -10000;
  for (int k = 0; k < 1000 && c[0] >> 20 & 3; k++) KeDelayExecutionThread(1, 0, &ms);
  CHECK(!(c[0] >> 20 & 3) && c[2] == 32 && !(c[4] >> 31) && c[1] >> 31 && !(c[0] >> 27));
  CHECK(XMAReleaseContext(va) == 0);
  MmFreePhysicalMemory(0, memory);
}

// ---- xam ----------------------------------------------------------------------
static void Xam(void) {
  CHECK(XGetGameRegion() == 0x00FF && XamGetSystemVersion() == 0x20449700);
  CHECK(XGetLanguage() == 1 && XNetLogonGetTitleID(7) == 0);
  u32 info = 9;  // the title has no execution info
  CHECK(XamGetExecutionId(&info) == kStatusNotFound && info == 0);
  u32 size = 1;
  char buf[16];
  CHECK(XamLoaderGetLaunchDataSize(&size) == 0x490 && size == 0 && XamLoaderGetLaunchData(buf, 16) == 0x490);
  uint16_t wsa[0xC8];
  CHECK(NetDll_WSAStartup(1, 0x0202, wsa) == 0 && wsa[0] == 0x0202 && wsa[1] == 0x0202);
  CHECK(NetDll_WSAStartup(3, 0x0202, wsa) == 10022 && NetDll_XNetStartup(1, 0) == 0);
  CHECK(NetDll_XNetGetEthernetLinkStatus(1) == 0);
  // No controller (player 4 has none): zeroed results,
  // ERROR_DEVICE_NOT_CONNECTED.
  uint8_t pad[0x14];
  memset(pad, 0xAA, sizeof pad);
  CHECK(XamInputGetCapabilities(3, 0, pad) == 1167 && pad[0] == 0 && pad[0x13] == 0);
  memset(pad, 0xAA, sizeof pad);
  CHECK(XamInputGetState(3, 0, pad) == 1167 && pad[0] == 0 && pad[0xF] == 0 && pad[0x10] == 0xAA);
  uint16_t vibration[2] = {0x8000, 0x8000};
  CHECK(XamInputSetState(3, 0, vibration) == 1167);
  // The Ex forms, which titles find through XexGetProcedureAddress.
  u32 xam = 0, wsa_ex = 0, xnet_ex = 0;
  CHECK(XexGetModuleHandle("xam.xex", &xam) == 0 && XexGetProcedureAddress(xam, 36, &wsa_ex) == 0 &&
        XexGetProcedureAddress(xam, 80, &xnet_ex) == 0);
  wsa[0] = wsa[1] = 0;
  CHECK(((u32(*)(u32, u32, void*, u32))(uintptr_t)wsa_ex)(1, 0x0101, wsa, 0x200DC900) == 0 && wsa[0] == 0x0101 &&
        wsa[1] == 0x0202);
  CHECK(((u32(*)(u32, u32, void*, u32))(uintptr_t)wsa_ex)(0, 0x0202, wsa, 0x200DC900) == 10022);
  CHECK(((u32(*)(u32, void*, u32))(uintptr_t)xnet_ex)(1, 0, 0x200DC900) == 0 &&
        ((u32(*)(u32, void*, u32))(uintptr_t)xnet_ex)(5, 0, 0x200DC900) == 10022);
  u32 p = 0;
  CHECK(XamAlloc(0, 0x440, &p) == 0 && p && *At(p) == 0 && *At(p + 0x43C) == 0);
  XamFree(p);
  // Memory statistics: the totals, what is left.
  u32 stats[26];
  stats[0] = 0x68;
  CHECK(MmQueryStatistics(stats) == 0 && stats[1] == 0x20000 && stats[3] > 0 && stats[3] < 0x20000 && stats[5] > 0);
  stats[0] = 0x60;
  CHECK(MmQueryStatistics(stats) == 0xC0000023);
  // Reserved virtual bytes are at most the title's total.
  u32 vbase = 0, vsize = 0x30000000, none = 0;
  CHECK(NtAllocateVirtualMemory(&vbase, &vsize, kMemReserve, kPageReadWrite, 0) == 0);
  stats[0] = 0x68;
  CHECK(MmQueryStatistics(stats) == 0 && stats[5] == stats[4]);
  CHECK(NtFreeVirtualMemory(&vbase, &none, kMemRelease, 0) == 0);
  // The music player's playback controller, in-process and as a request.
  u32 controller = 9, locked = 9;
  u32 get[3] = {2, (u32)(uintptr_t)&controller, (u32)(uintptr_t)&locked};
  CHECK(XMsgInProcessCall(0xFA, 0x7001B, get, 0) == 0 && controller == 0 && locked == 0);
  u32 logon = 0;
  CHECK(XMsgInProcessCall(0xFC, 0x58004, &logon, 0) == 0 && logon == 1);
  u32 context[6] = {0, 0, 0, 0, 0x8001, 3};  // XUserSetContextEx: user 0, presence 3
  CHECK(XMsgInProcessCall(0xFB, 0xB0006, context, (void*)sizeof context) == 0);
  u32 request[4] = {0};
  CHECK(XMsgInProcessCall(0xFC, 0x58020, (void*)(uintptr_t)logon, request) == 0x80070012);
  u32 set[3] = {2, 0, 1};
  CHECK(XMsgStartIORequestEx(0xFA, 0x7001A, 0, set, sizeof set, 0) == 0);
  u32 e = 0, ov[7] = {0, 0, 0, 0, 0, 0, 0};
  CHECK(NtCreateEvent(&e, 0, 0, 0) == 0);
  ov[3] = e;
  int64_t zero = 0;
  CHECK(XMsgStartIORequest(0xFA, 0x7001A, ov, set, sizeof set) == 0 && ov[0] == 0 &&
        NtWaitForSingleObjectEx(e, 1, 0, &zero) == 0);
  // A refused request: its result and extended error, or the last error.
  set[1] = 1;
  CHECK(XMsgStartIORequest(0xFA, 0x7001A, ov, set, sizeof set) == 0 && ov[0] == 87 && ov[6] == 0x80070057);
  CHECK(XMsgStartIORequest(0xFA, 0x7001A, 0, set, sizeof set) == 0 &&
        *At(*At((u32)pcr() + 0x100) + 0x160) == 0x80070057);
  // Requests complete when started: nothing to cancel or wait for.
  CHECK(XMsgCancelIORequest(ov, 1) == 0 && ov[0] == 87);
  NtClose(e);
}

// ---- users, profile settings, notifications ------------------------------------
// tests/CMakeLists.txt runs the title with the default profile, "Player",
// signed in as player 1.
static void Users(void) {
  CHECK(XamUserGetSigninState(0) == 1 && XamUserGetSigninState(1) == 0);
  uint64_t xuid = 1;
  CHECK(XamUserGetXUID(0, 1, &xuid) == 0 && xuid == 0xE000000000000001ull);
  CHECK(XamUserGetXUID(0, 2, &xuid) == 0x80070525 && xuid == 0);
  CHECK(XamUserGetXUID(5, 1, &xuid) == 0x80070057);
  char name[16];
  CHECK(XamUserGetName(0, name, sizeof name) == 0 && Memeq(name, "Player", 7));
  CHECK(XamUserGetName(0, name, 4) == 0 && Memeq(name, "Pla", 4));
  CHECK(XamUserGetName(1, name, sizeof name) == 0x525 && name[0] == 0);
  // A profile signed in only locally holds no privilege.
  u32 allowed = 9;
  CHECK(XamUserCheckPrivilege(0, 254, &allowed) == 1245 && allowed == 0);
  CHECK(XamUserCheckPrivilege(0xFF, 243, &allowed) == 1245 && allowed == 0);
  CHECK(XamUserCheckPrivilege(1, 243, &allowed) == 1317 && XamUserCheckPrivilege(4, 243, &allowed) == 87);
  // Friends are Live's.
  uint64_t friend_xuid = 0x0009000000000001ull;
  u32 is_friend = 9;
  CHECK(XamUserAreUsersFriends(0, &friend_xuid, 1, &is_friend, 0) == 1245 && is_friend == 0);
  CHECK(XamUserAreUsersFriends(1, &friend_xuid, 1, &is_friend, 0) == 1317 &&
        XamUserAreUsersFriends(4, &friend_xuid, 1, &is_friend, 0) == 87);

  // Profile settings: the size first, then the settings.
  u32 ids[4] = {0x1004000C, 0x1004000D, 0x1004000E, 0x10040003}, size = 0;
  CHECK(XamUserReadProfileSettings9(0xFFFE07D1, 0, 0, 0, 4, ids, &size, 0, 0) == 0x7A && size == 4 * 0x28 + 8);
  static u32 results[(4 * 0x28 + 8) / 4];
  CHECK(XamUserReadProfileSettings9(0xFFFE07D1, 0, 0, 0, 4, ids, &size, results, 0) == 0);
  u32 settings = (u32)(uintptr_t)results + 8;
  CHECK(results[0] == 4 && results[1] == settings);
  CHECK(*At(settings) == 0 && *At(settings + 0x10) == 0x1004000C && (*At(settings + 0x18) >> 24) == 1);
  CHECK(*At(settings + 3 * 0x28) == 1 && *At(settings + 3 * 0x28 + 0x20) == 3);
  u32 ov[7] = {0, 0, 0, 0, 0, 0, 0};
  CHECK(XamUserReadProfileSettings9(0xFFFE07D1, 0, 0, 0, 4, ids, &size, results, ov) == 0x3E5 && ov[0] == 0);
  CHECK(XamUserReadProfileSettings9(0xFFFE07D1, 0, 0, 0, 0, ids, &size, results, 0) == 0x57);
  u32 gamer[2] = {0x10040002, 0x10040018};  // y axis inverted, control sensitivity: not written
  size = 2 * 0x28 + 8;
  CHECK(XamUserReadProfileSettings9(0, 0, 0, 0, 2, gamer, &size, results, 0) == 0 && results[0] == 2 &&
        *At(settings) == 0 && *At(settings + 0x28) == 0 && *At(settings + 0x28 + 0x10) == 0x10040018);
  u32 card[3] = {0x10040022, 0x402C0011, 0x5004000B};  // auto aim, motto, reputation: not written
  size = 3 * 0x28 + 0x2C + 8;
  CHECK(XamUserReadProfileSettings9(0, 0, 0, 0, 3, card, &size, results, 0) == 0 && results[0] == 3 &&
        *At(settings) == 0 && *At(settings + 0x28) == 0 && *At(settings + 0x28 + 0x20) == 0 &&
        *At(settings + 0x50) == 0 && *At(settings + 0x50 + 0x10) == 0x5004000B);

  // Title settings written as a title saves them: the three title-specific
  // settings at once, the last one empty with an address (a zero-length
  // copy into an empty buffer has corrupted heaps elsewhere), then read
  // back, written again and read again. tests/CMakeLists.txt checks the
  // saved files.
  static uint8_t big[1000], mid[214], small2[10];
  for (u32 k = 0; k < sizeof big; k++) big[k] = (uint8_t)k;
  for (u32 k = 0; k < sizeof mid; k++) mid[k] = (uint8_t)(0xA5 ^ k);
  for (u32 k = 0; k < sizeof small2; k++) small2[k] = (uint8_t)(0x30 + k);
  struct {
    u32 source, pad, user, user_low, id, pad2, type, pad3, size, ptr;
  } w[3];
  memset(w, 0, sizeof w);
  const u32 tids[3] = {0x63E83FFF, 0x63E83FFE, 0x63E83FFD};
  u32 lens[3] = {sizeof big, sizeof mid, 0}, ptrs[3] = {(u32)(uintptr_t)big, (u32)(uintptr_t)mid, (u32)(uintptr_t)mid};
  for (int k = 0; k < 3; k++) {
    w[k].id = tids[k];
    w[k].type = 6u << 24;
    w[k].size = lens[k];
    w[k].ptr = ptrs[k];
  }
  CHECK(XamUserWriteProfileSettings(0x54540001, 0, 3, w, 0) == 0);
  static u32 tr[(3 * 0x28 + 3000 + 8) / 4];
  u32 trsize = sizeof tr;
  CHECK(XamUserReadProfileSettings9(0x54540001, 0, 0, 0, 3, (u32*)tids, &trsize, tr, 0) == 0);
  u32 ts = (u32)(uintptr_t)tr + 8;
  CHECK(*At(ts) == 2 && *At(ts + 0x20) == 1000 && Memeq((void*)(uintptr_t)*At(ts + 0x24), big, 1000));
  CHECK(*At(ts + 0x28) == 2 && *At(ts + 0x28 + 0x20) == 214 &&
        Memeq((void*)(uintptr_t)*At(ts + 0x28 + 0x24), mid, 214));
  CHECK(*At(ts + 0x50) == 2 && *At(ts + 0x50 + 0x20) == 0);
  w[1].size = sizeof small2;
  w[1].ptr = (u32)(uintptr_t)small2;
  CHECK(XamUserWriteProfileSettings(0x54540001, 0, 2, &w[1], 0) == 0);
  CHECK(XamUserReadProfileSettings9(0x54540001, 0, 0, 0, 3, (u32*)tids, &trsize, tr, 0) == 0);
  CHECK(*At(ts + 0x28 + 0x20) == 10 && Memeq((void*)(uintptr_t)*At(ts + 0x28 + 0x24), small2, 10) &&
        *At(ts + 0x50 + 0x20) == 0 && *At(ts + 0x20) == 1000);
  // The profile's own settings a title writes are the profile's; a title
  // may not write another kind.
  w[0].id = 0x10040002;
  w[0].type = 1u << 24;
  w[0].size = 1;
  CHECK(XamUserWriteProfileSettings(0x54540001, 0, 1, w, 0) == 0);
  u32 inv = 0x10040002;
  trsize = sizeof tr;
  CHECK(XamUserReadProfileSettings9(0xFFFE07D1, 0, 0, 0, 1, &inv, &trsize, tr, 0) == 0 && *At(ts) == 2 &&
        *At(ts + 0x20) == 1);
  w[0].id = 0x10040100;
  CHECK(XamUserWriteProfileSettings(0x54540001, 0, 1, w, 0) == 0x57);
  // An awarded gamer picture becomes the picture key; none for a slot
  // without a profile.
  CHECK(XamWriteGamerTile(0, 0x54540001, 0x10002, 0x20002, 1, 0) == 0);
  CHECK(XamWriteGamerTile(1, 0x54540001, 0x10002, 0x20002, 1, 0) == 1627);
  u32 key = 0x4064000F;
  trsize = sizeof tr;
  CHECK(XamUserReadProfileSettings9(0xFFFE07D1, 0, 0, 0, 1, &key, &trsize, tr, 0) == 0 && *At(ts) == 2 &&
        *At(ts + 0x20) == 50);
  const uint16_t* picture = (const uint16_t*)(uintptr_t)*At(ts + 0x24);
  const char* expected = "545400010001000200020002";
  int same = 1;
  for (int k = 0; k < 25; k++) same &= picture[k] == (uint16_t)expected[k];
  CHECK(same);
  // Statistics are Live's: an enumerator sized for its rows and columns,
  // whose calls fail.
  struct {
    u32 view, columns;
    uint16_t ids[64];
  } view = {1, 2, {1, 2}};
  u32 stats_size = 0, stats = 0, rows = 9;
  CHECK(XamUserCreateStatsEnumerator(0x54540001, 0, 1, 10, 1, &view, &stats_size, &stats) == 0 &&
        stats_size == (10 * 0x30 + 0x10) + 8 + 2 * 10 * 0x1C);
  static uint8_t stats_buf[0x428];
  memset(stats_buf, 0xAA, sizeof stats_buf);
  CHECK(XamEnumerate(stats, 0, stats_buf, stats_size, &rows, 0) == 1627 && stats_buf[0] == 0 &&
        stats_buf[stats_size - 1] == 0);
  NtClose(stats);
  CHECK(XamUserCreateStatsEnumerator(0x54540001, 0, 1, 101, 1, &view, &stats_size, &stats) == 87);

  // A new listener holds what the system has pending: the UI is not
  // showing, player 1 signed in; a Live listener: disconnected, no link.
  u32 h = XamNotifyCreateListener(1, 1), id = 0, value = 9;
  CHECK(h != 0 && XNotifyGetNext(h, 0, &id, &value) == 1 && id == 9 && value == 0);
  CHECK(XNotifyGetNext(h, 0, &id, &value) == 1 && id == 0xA && value == 1);
  CHECK(XNotifyGetNext(h, 0, &id, &value) == 0 && id == 0);
  int64_t zero = 0;
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, &zero) == kStatusTimeout);
  // The sign-in UI: none for online-enabled profiles only; otherwise it
  // shows and closes with nothing changed, which the listener sees as
  // XN_SYS_UI 0 (its latest value), no sign-in change.
  CHECK(XamShowSigninUI(1, 0x400000) == 1627);
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, &zero) == kStatusTimeout);
  CHECK(XamShowSigninUI(1, 0) == 0);
  CHECK(XNotifyGetNext(h, 0, &id, &value) == 1 && id == 9 && value == 0);
  CHECK(XNotifyGetNext(h, 0, &id, &value) == 0);
  NtClose(h);
  h = XamNotifyCreateListener(2, 1);
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, &zero) == 0);
  CHECK(XNotifyGetNext(h, 0x02000003, &id, &value) == 1 && id == 0x02000003 && value == 0);
  CHECK(XNotifyGetNext(h, 0, &id, &value) == 1 && id == 0x02000001 && value == 0x001510F1);
  NtClose(h);
  CHECK(XNotifyPositionUI(1) == 0);

  // The system UI: the Live ones are refused, the others show and close
  // (XN_SYS_UI 1, then 0: a listener holds 0).
  CHECK(XamShowFriendsUI(0) == 5 && XamShowPlayerReviewUI(0, 0x0009000000000001ull) == 5 &&
        XamShowPlayerReviewUI(4, 0) == 87);
  CHECK(XamShowGamerCardUIForXUID(0, 0x0009000000000001ull, 0) == 5 &&
        XamShowGamerCardUIForXUID(0, 0xE000000000000002ull, 0) == 87);
  CHECK(XamShowMarketplaceUI(1, 0, 0, 0) == 1317 && XamShowMarketplaceUI(0, 0x19, 0, 0) == 87);
  h = XamNotifyCreateListener(1, 1);
  while (XNotifyGetNext(h, 0, &id, &value)) {
  }
  CHECK(XamShowAchievementsUI(0, 0) == 0 && XamShowGamerCardUIForXUID(0, 0xE000000000000001ull, 0) == 0 &&
        XamShowMarketplaceUI(0, 0, 0, 0) == 0 && XamShowDirtyDiscErrorUI() == 0);
  CHECK(XNotifyGetNext(h, 9, &id, &value) == 1 && value == 0 && XNotifyGetNext(h, 0, &id, &value) == 0);
  // The keyboard gives its default text back, truncated; the message box
  // its focused button.
  static const uint16_t text[] = {'a', 'b', 'c', 'd', 0}, ok[] = {'O', 'K', 0}, no[] = {'N', 'o', 0};
  uint16_t typed[8] = {9, 9, 9, 9, 9, 9, 9, 9};
  u32 kov[7] = {0, 0, 0, 0, 0, 0, 0};
  CHECK(XamShowKeyboardUI(0, 0, text, 0, 0, typed, 3, kov) == 997 && kov[0] == 0 && typed[0] == 'a' &&
        typed[1] == 'b' && typed[2] == 0 && typed[3] == 9);
  const uint16_t* buttons[2] = {ok, no};
  u32 pressed[2] = {9, 9};
  CHECK(XamShowMessageBoxUIEx9(0, text, text, 2, buttons, 1, 0, pressed, kov) == 997 && kov[0] == 0 &&
        pressed[0] == 1);
  CHECK(XamShowMessageBoxUIEx9(0, text, text, 2, buttons, 1, 0, 0, 0) == 0);
  CHECK(XamShowMessageBoxUIEx9(0, text, text, 2, buttons, 2, 0, pressed, kov) == 87 && kov[6] == 87);
  CHECK(XamShowMessageBoxUIEx9(0, text, text, 2, buttons, 1, 0, pressed, 0) == 87 &&
        XamShowMessageBoxUIEx9(0, text, text, 2, buttons, 1, 0x100000, pressed, kov) == 87);
  NtClose(h);

  // Voice: no headset.
  u32 voice = 0;
  CHECK(XamVoiceCreate(0, 3, &voice) == 0 && voice && *At(voice + 4) == 3 && *At(voice + 8) != 0);
  u32 packet = *At(voice + 0xC), packets = 0;
  for (; packet; packet = *At(packet + 0xC)) packets += *At(packet + 8) == voice;
  CHECK(packets == 3 && *At(*At(voice + 0x10) + 0xC) == 0);
  uint8_t talk[16] = {0};
  CHECK(XamVoiceHeadsetPresent(voice) == 0 && XamVoiceSubmitPacket(voice, sizeof talk, talk) == 0);
  CHECK(XamVoiceClose(voice) == 1 && *At(voice + 4) == 0 && *At(voice + 8) == 0);
}

// ---- content ------------------------------------------------------------------
struct ContentData {  // XCONTENT_DATA
  u32 device, type;
  uint16_t display[128];
  char file[42];
  uint8_t pad[2];
};

static void Data(struct ContentData* d, u32 device, const char* file, const char* display) {
  memset(d, 0, sizeof *d);
  d->device = device;
  d->type = 1;  // a saved game
  for (int k = 0; display[k]; k++) d->display[k] = (uint16_t)display[k];
  for (int k = 0; file[k]; k++) d->file[k] = file[k];
}

static void Content(void) {
  // The hard disk, device 1.
  CHECK(XamContentGetDeviceState(1, 0) == 0 && XamContentGetDeviceState(2, 0) == 0x48F);
  struct {
    u32 id, type;
    uint64_t total, free;
    uint16_t name[28];
  } device;
  CHECK(XamContentGetDeviceData(1, &device) == 0 && device.id == 1 && device.type == 1 && device.total > 0 &&
        device.free <= device.total && device.name[0] == 'H' && device.name[9] == 'e' && device.name[10] == 0);
  CHECK(XamContentGetDeviceData(2, &device) == 0x48F);
  // The device selector picks it, through an XOVERLAPPED only.
  u32 chosen = 0, sel[7] = {9, 9, 0, 0, 0, 0, 9};
  CHECK(XamShowDeviceSelectorUI(0, 1, 0, 0x10000, &chosen, 0) == 0x57 && chosen == 0);
  CHECK(XamShowDeviceSelectorUI(4, 1, 0, 0x10000, &chosen, sel) == 0x57 && sel[6] == 0x57 && chosen == 0);
  CHECK(XamShowDeviceSelectorUI(0xFF, 1, 0x200, 0x10000, &chosen, sel) == 0x3E5 && sel[0] == 0 &&
        sel[6] == 0 && chosen == 1);

  // No saves yet.
  static struct ContentData items[4];
  u32 size = 0, e = 0, n = 9;
  CHECK(XamContentCreateEnumerator(0, 1, 1, 0, 2, &size, &e) == 0 && size == 2 * sizeof(struct ContentData));
  CHECK(XamEnumerate(e, 0, items, size, &n, 0) == 0x12 && n == 0);
  NtClose(e);
  CHECK(XamEnumerate(e, 0, items, size, &n, 0) == 6);

  // A save made, written and closed.
  struct ContentData d, d2, d3;
  Data(&d, 1, "slot1", "Save One");
  u32 disposition = 9, license = 9, io[2], h = 0;
  char buf[16];
  CHECK(XamContentCreateEx9(0, "save", &d, 3, &disposition, &license, 0, 0, 0) == 3 && disposition == 0);
  CHECK(XamContentCreateEx9(0, "save", &d, 1, &disposition, &license, 0, 0, 0) == 0 && disposition == 1 &&
        license == 0);
  CHECK(OpenPath(&h, "save:\\game.dat", kGenericWrite, kCreate, kSync | kNonDirectoryFile, io) == 0);
  CHECK(NtWriteFile(h, 0, 0, 0, io, "progress", 8, 0) == 0);
  NtClose(h);
  CHECK(XamContentClose("save", 0) == 0 && XamContentClose("save", 0) == 2);
  CHECK(OpenPath(&h, "save:\\game.dat", kGenericRead, kOpen, kSync, io) == kStatusObjectPathNotFound);
  // Opened again through an XOVERLAPPED: complete at once, the disposition
  // its length.
  u32 ov[7] = {9, 9, 0, 0, 0, 0, 9};
  CHECK(XamContentCreateEx9(0, "save", &d, 4, &disposition, 0, 0, 0, ov) == 0x3E5 && ov[0] == 0 && ov[1] == 2 &&
        ov[6] == 0 && disposition == 2);
  CHECK(OpenPath(&h, "save:\\game.dat", kGenericRead, kOpen, kSync, io) == 0);
  CHECK(NtReadFile(h, 0, 0, 0, io, buf, 16, 0) == 0 && io[1] == 8 && Memeq(buf, "progress", 8));
  NtClose(h);
  // Refused: the root in use, the save as new, deleting it while open,
  // names xam does not take, a root that is no package.
  Data(&d2, 0, "slot2", "Save Two");
  CHECK(XamContentCreateEx9(0, "save", &d2, 1, &disposition, 0, 0, 0, 0) == 0x34);
  CHECK(XamContentCreateEx9(0, "other", &d, 1, &disposition, 0, 0, 0, 0) == 0xB7);
  CHECK(XamContentDelete(0, &d, 0) == 0x20);
  CHECK(XamContentCreateEx9(0, "sa.ve", &d2, 1, &disposition, 0, 0, 0, 0) == 0x7B);
  d2.file[0] = 0;
  CHECK(XamContentCreateEx9(0, "save2", &d2, 1, &disposition, 0, 0, 0, 0) == 0x57);
  d2.file[0] = 's';
  CHECK(XamContentClose("game", 0) == 0x57);
  // Player 1 made it.
  u32 is_creator = 9;
  uint64_t creator = 0;
  CHECK(XamContentGetCreator(0, &d, &is_creator, &creator, 0) == 0 && is_creator == 1 &&
        creator == 0xE000000000000001ull);
  CHECK(XamContentClose("save", 0) == 0);
  // TRUNCATE_EXISTING empties it.
  CHECK(XamContentCreateEx9(0, "save", &d, 5, &disposition, 0, 0, 0, 0) == 0 && disposition == 2);
  CHECK(OpenPath(&h, "save:\\game.dat", kGenericRead, kOpen, kSync, io) == kStatusObjectNameNotFound);
  CHECK(OpenPath(&h, "save:\\game.dat", kGenericWrite, kCreate, kSync | kNonDirectoryFile, io) == 0);
  CHECK(NtWriteFile(h, 0, 0, 0, io, "again", 5, 0) == 0);
  NtClose(h);
  CHECK(XamContentClose("save", 0) == 0);

  // Another save (on any device: the hard disk), and one no user owns.
  CHECK(XamContentCreateEx9(0, "save", &d2, 1, &disposition, 0, 0, 0, 0) == 0 && disposition == 1);
  CHECK(XamContentClose("save", 0) == 0);
  Data(&d3, 1, "common", "Shared");
  CHECK(XamContentCreateEx9(0xFF, "save", &d3, 1, &disposition, 0, 0, 0, 0) == 0 && disposition == 1);
  CHECK(XamContentClose("save", 0) == 0);
  // Listed: the player's by name, then the one no user owns, its name
  // marked.
  CHECK(XamContentCreateEnumerator(0, 0, 1, 0, 4, &size, &e) == 0);
  CHECK(XamEnumerate(e, 0, items, size, &n, 0) == 0 && n == 2 && items[0].device == 1 && items[0].type == 1 &&
        Memeq(items[0].file, "slot1", 6) && items[0].display[0] == 'S' && items[0].display[5] == 'O' &&
        Memeq(items[1].file, "slot2", 6));
  CHECK(XamEnumerate(e, 0, items, size, &n, 0) == 0 && n == 1 && (uint8_t)items[0].file[0] == ('c' | 0x80) &&
        Memeq(items[0].file + 1, "ommon", 6));
  CHECK(XamEnumerate(e, 0, items, size, &n, 0) == 0x12 && n == 0);
  NtClose(e);
  // The marked name opens that one.
  CHECK(XamContentCreateEx9(0, "save", &items[0], 3, &disposition, 0, 0, 0, 0) == 0 && disposition == 2);
  CHECK(XamContentClose("save", 0) == 0);
  // One a call through an XOVERLAPPED, without those no user owns
  // (XCONTENTFLAG_ENUM_EXCLUDECOMMON); the end is ERROR_FUNCTION_FAILED with
  // ERROR_NO_MORE_FILES as the extended error.
  CHECK(XamContentCreateEnumerator(0, 1, 1, 0x1000, 1, &size, &e) == 0 && size == sizeof(struct ContentData));
  CHECK(XamEnumerate(e, 0, items, size, 0, ov) == 0x3E5 && ov[0] == 0 && ov[1] == 1 &&
        Memeq(items[0].file, "slot1", 6));
  CHECK(XamEnumerate(e, 0, items, size, 0, ov) == 0x3E5 && ov[1] == 1 && Memeq(items[0].file, "slot2", 6));
  CHECK(XamEnumerate(e, 0, items, size, 0, ov) == 0x3E5 && ov[0] == 0x65B && ov[1] == 0 && ov[6] == 0x80070012);
  NtClose(e);
  // Deleted (by its device: 0 names none).
  CHECK(XamContentDelete(0, &d2, 0) == 0x48F);
  d2.device = 1;
  CHECK(XamContentDelete(0, &d2, 0) == 0 && XamContentDelete(0, &d2, 0) == 2);
  // Player 2 is not signed in.
  CHECK(XamContentCreateEnumerator(1, 0, 1, 0, 1, &size, &e) == 0x525);
}

// ---- timers --------------------------------------------------------------------
static void Timers(void) {
  u32 t = 0, previous = 9;
  int64_t zero = 0, due = -200000, fifty_ms = -500000;  // 20 ms
  CHECK(NtCreateTimer(&t, 0, 0) == 0);
  CHECK(NtSetTimerEx(t, &due, 0, 0, 0, 0, 0, &previous) == 0 && previous == 0);
  CHECK(NtWaitForSingleObjectEx(t, 1, 0, &zero) == kStatusTimeout);
  CHECK(NtWaitForSingleObjectEx(t, 1, 0, &fifty_ms) == 0);
  // A notification timer stays signalled.
  CHECK(NtWaitForSingleObjectEx(t, 1, 0, &zero) == 0);
  NtClose(t);
  // A periodic synchronization timer: a signal every 10 ms, one per wait.
  u32 s = 0, current = 9;
  int64_t ten_ms = -100000;
  CHECK(NtCreateTimer(&s, 0, 1) == 0 && NtSetTimerEx(s, &ten_ms, 0, 0, 0, 0, 10, 0) == 0);
  int fired = 0;
  for (int k = 0; k < 5; k++) fired += NtWaitForSingleObjectEx(s, 1, 0, &fifty_ms) == 0;
  CHECK(fired == 5);
  CHECK(NtCancelTimer(s, &current) == 0 && current == 0);
  CHECK(NtWaitForSingleObjectEx(s, 1, 0, &fifty_ms) == kStatusTimeout);
  NtClose(s);
}

// ---- XGI: contexts, achievements, sessions ---------------------------------
static u32 Xgi(u32 message, u32* request) { return XMsgInProcessCall(0xFB, message, request, 0); }

static void Sessions(void) {
  // Achievements, kept with the profile: S_FALSE once already earned.
  u32 earned[2] = {0, 5}, list[2] = {1, (u32)(uintptr_t)earned};
  CHECK(Xgi(0xB0008, list) == 0 && Xgi(0xB0008, list) == 1);
  earned[0] = 1;
  CHECK(Xgi(0xB0008, list) == 0x80070490);

  u32 h = 0, obj = 0, e = 0;
  CHECK(XamSessionCreateHandle(&h) == 0 && XamSessionRefObjByHandle(h, &obj) == 0 && obj && *At(obj) == 0);
  CHECK(NtCreateEvent(&e, 0, 0, 0) == 0 && XamSessionRefObjByHandle(e, &obj) == 6);
  NtClose(e);
  u32 info[15], nonce[2];
  // {object, flags, public slots, private slots, user, *info, *nonce}
  u32 create[7] = {obj, 2, 1, 1, 0, (u32)(uintptr_t)info, (u32)(uintptr_t)nonce};
  CHECK(Xgi(0xB0010, create) == 0x80155209);  // presence needs Live
  create[1] = 0x20 | 0x10;
  CHECK(Xgi(0xB0010, create) == 0x80155201);  // system link with arbitration
  create[1] = 0;
  create[4] = 1;
  CHECK(Xgi(0xB0010, create) == 0x80070490);  // no such user
  create[4] = 0;
  CHECK(Xgi(0xB0010, create) == 0x80155205 && *At(obj) == 0);  // no game type yet
  u32 context[6] = {0, 0, 0, 0, 0x800A, 1};
  CHECK(Xgi(0xB0006, context) == 0);
  CHECK(Xgi(0xB0010, create) == 0 && *At(obj) != 0);
  // Members: a local user takes its slot once; a remote one finds none left.
  u32 slots[1] = {0}, want[1] = {0};
  u32 join[5] = {obj, 1, 0, (u32)(uintptr_t)slots, (u32)(uintptr_t)want};
  CHECK(Xgi(0xB0012, join) == 0 && Xgi(0xB0012, join) == 0);
  uint64_t remote[1] = {0xE000000000000002ull};
  u32 join_remote[5] = {obj, 1, (u32)(uintptr_t)remote, 0, (u32)(uintptr_t)want};
  CHECK(Xgi(0xB0012, join_remote) == 0x80155202);
  want[0] = 1;
  CHECK(Xgi(0xB0012, join_remote) == 0);  // a private slot
  u32 both[5] = {obj, 1, (u32)(uintptr_t)remote, (u32)(uintptr_t)slots, 0};
  CHECK(Xgi(0xB0012, both) == 0x80070057);
  // Start and end.
  u32 session[3] = {obj, 0, 0};
  CHECK(Xgi(0xB0014, session) == 0 && Xgi(0xB0014, session) == 0x80155206);
  CHECK(Xgi(0xB0025, session) == 0x80155204);  // no statistics
  CHECK(Xgi(0xB0026, session) == 0);
  CHECK(Xgi(0xB0015, session) == 0 && Xgi(0xB0015, session) == 0x80155206);
  CHECK(Xgi(0xB0026, session) == 0x80155206);
  u32 modify[4] = {obj, 2, 1, 1};
  CHECK(Xgi(0xB0018, modify) == 0x80155204);
  modify[1] = 0x100;
  CHECK(Xgi(0xB0018, modify) == 0);
  u32 leave[4] = {obj, 1, 0, (u32)(uintptr_t)slots};
  CHECK(Xgi(0xB0013, leave) == 0 && Xgi(0xB0012, join) == 0);  // its slot free again
  CHECK(Xgi(0xB0011, session) == 0 && Xgi(0xB0014, session) == 0x80155206);
  ObDereferenceObject(obj);
  ObDereferenceObject(obj);
  NtClose(h);
  // Live's queries fail with no connection.
  u32 stats[7] = {0, 0, 0, 0, 0, 8, (u32)(uintptr_t)nonce};
  nonce[0] = nonce[1] = 9;
  CHECK(Xgi(0xB0021, stats) == 0x80151802 && nonce[0] == 0 && nonce[1] == 0);
}

// ---- APCs ------------------------------------------------------------------
// What the APC routines saw, in the order they ran.
static volatile u32 apc_log[16], apc_count;
void NormalApc(u32 context, u32 arg1, u32 arg2) {
  if (apc_count + 3 <= 16) {
    apc_log[apc_count++] = context;
    apc_log[apc_count++] = arg1;
    apc_log[apc_count++] = arg2;
  }
}
// A kernel routine that changes the normal routine's context.
void KernelApc(u32 apc, u32* normal, u32* context, u32* arg1, u32* arg2) {
  (void)apc, (void)normal, (void)arg2;
  *context = *context + 0x100;
  *arg1 = *arg1 + 1;
}

static u32 AlertableWaiter(void* event) {
  int64_t ten_s = -100000000;
  return KeWaitForSingleObject(event, 3, 1, 1, &ten_s);
}

static void Apcs(void) {
  u32 me = *At((u32)pcr() + 0x100);
  int64_t zero = 0;
  // A user APC runs in an alertable user-mode wait only, which then
  // returns STATUS_USER_APC.
  apc_count = 0;
  CHECK(NtQueueApcThread(0xFFFFFFFE, (void*)normal_entry, 1, 2, 3) == 0);
  CHECK(KeDelayExecutionThread(1, 0, &zero) == 0 && apc_count == 0);
  CHECK(KeDelayExecutionThread(0, 1, &zero) == 0 && apc_count == 0);  // kernel mode
  CHECK(KeDelayExecutionThread(1, 1, &zero) == 0xC0 && apc_count == 3 && apc_log[0] == 1 && apc_log[1] == 2 &&
        apc_log[2] == 3);
  CHECK(KeDelayExecutionThread(1, 1, &zero) == 0);  // none left
  // A signalled object comes before the APC.
  u32 e = 0;
  CHECK(NtCreateEvent(&e, 0, 0, 1) == 0);
  CHECK(NtQueueApcThread(0xFFFFFFFE, (void*)normal_entry, 4, 5, 6) == 0);
  CHECK(NtWaitForSingleObjectEx(e, 1, 1, &zero) == 0 && apc_count == 3);
  u32 unset = 0;  // a kernel-mode wait runs no user APC, alertable or not
  CHECK(NtCreateEvent(&unset, 0, 0, 0) == 0);
  CHECK(NtWaitForSingleObjectEx(unset, 0, 1, &zero) == kStatusTimeout && apc_count == 3);
  NtClose(unset);
  CHECK(KeDelayExecutionThread(1, 1, &zero) == 0xC0 && apc_count == 6 && apc_log[3] == 4);
  NtClose(e);

  // A title's own KAPC: its kernel routine first, which may change the
  // normal routine's arguments; queued once at a time.
  u32 apc[10];
  apc_count = 0;
  KeInitializeApc(apc, me, (void*)kernel_entry, 0, (void*)normal_entry, 1, 7);
  CHECK((apc[0] >> 24) == 18 && ((apc[0] >> 8) & 0xFF) == 1);
  CHECK(KeInsertQueueApc(apc, 8, 9, 0) == 1 && KeInsertQueueApc(apc, 8, 9, 0) == 0);
  CHECK(KeDelayExecutionThread(1, 1, &zero) == 0xC0 && apc_count == 3 && apc_log[0] == 0x107 &&
        apc_log[1] == 9 && apc_log[2] == 9);
  CHECK((apc[0] & 0xFF) == 0);  // no longer inserted
  // A kernel APC runs in any wait, which goes on; not inside a critical
  // region, but as the region ends.
  apc_count = 0;
  KeInitializeApc(apc, me, (void*)kernel_entry, 0, (void*)normal_entry, 0, 0x10);
  KeEnterCriticalRegion();
  CHECK(KeInsertQueueApc(apc, 1, 2, 0) == 1);
  CHECK(KeDelayExecutionThread(1, 0, &zero) == 0 && apc_count == 0);
  KeLeaveCriticalRegion();
  CHECK(apc_count == 3 && apc_log[0] == 0x110 && apc_log[1] == 2);
  CHECK(KeInsertQueueApc(apc, 1, 2, 0) == 1);
  CHECK(KeDelayExecutionThread(1, 0, &zero) == 0 && apc_count == 6);

  // An APC to another thread wakes its alertable wait.
  u32 ev[4] = {0, 0, (u32)(uintptr_t)&ev[2], (u32)(uintptr_t)&ev[2]};
  u32 h = 0;
  apc_count = 0;
  CHECK(ExCreateThread(&h, 0, 0, (void*)thread_startup, (void*)AlertableWaiter, ev, 0) == 0);
  int64_t ten_ms = -100000;
  KeDelayExecutionThread(1, 0, &ten_ms);
  CHECK(NtQueueApcThread(h, (void*)normal_entry, 11, 12, 13) == 0);
  u32 obj = 0;
  CHECK(NtWaitForSingleObjectEx(h, 1, 0, 0) == 0 && apc_count == 3 && apc_log[0] == 11);
  CHECK(ObReferenceObjectByHandle(h, __imp_ExThreadObjectType, &obj) == 0 && *At(obj + 0x140) == 0xC0);
  ObDereferenceObject(obj);
  // Its thread has ended: nothing more is queued.
  CHECK(NtQueueApcThread(h, (void*)normal_entry, 1, 2, 3) != 0);
  NtClose(h);

  // A timer's APC: routine(context, the expiry's system time).
  u32 t = 0;
  apc_count = 0;
  CHECK(NtCreateTimer(&t, 0, 0) == 0);
  CHECK(NtSetTimerEx(t, &ten_ms, (void*)normal_entry, 1, (void*)(uintptr_t)21, 0, 0, 0) == 0);
  int64_t second = -10000000;
  CHECK(KeDelayExecutionThread(1, 1, &second) == 0xC0 && apc_count == 3 && apc_log[0] == 21 &&
        (apc_log[1] | apc_log[2]) != 0);
  // Cancelled before it runs, it does not.
  CHECK(NtSetTimerEx(t, &zero, (void*)normal_entry, 1, (void*)(uintptr_t)22, 0, 0, 0) == 0);
  KeDelayExecutionThread(1, 0, &ten_ms);
  u32 current = 0;
  CHECK(NtCancelTimer(t, &current) == 0);
  CHECK(KeDelayExecutionThread(1, 1, &zero) == 0 && apc_count == 3);
  NtClose(t);

  // An I/O's APC routine: routine(context, status block, 0), unless the
  // I/O failed.
  u32 f = 0, io[2];
  char buf[4];
  apc_count = 0;
  CHECK(OpenPath(&f, "game:\\data.bin", kGenericRead, kOpen, kSync | kNonDirectoryFile, io) == 0);
  CHECK(NtReadFile(f, 0, (void*)normal_entry, (void*)(uintptr_t)31, io, buf, 4, 0) == 0);
  CHECK(KeDelayExecutionThread(1, 1, &zero) == 0xC0 && apc_count == 3 && apc_log[0] == 31 &&
        apc_log[1] == (u32)(uintptr_t)io && apc_log[2] == 0);
  int64_t end = 32;
  CHECK(NtReadFile(f, 0, (void*)normal_entry, (void*)(uintptr_t)32, io, buf, 4, &end) == kStatusEndOfFile);
  CHECK(KeDelayExecutionThread(1, 1, &zero) == 0 && apc_count == 3);
  NtClose(f);

  // An XMsg request's completion routine, in place of its event:
  // routine(result, length, overlapped).
  u32 set[3] = {2, 1, 1}, ov[7] = {0, 0, 0, 0, (u32)(uintptr_t)normal_entry, 0, 0};
  apc_count = 0;
  CHECK(XMsgStartIORequest(0xFA, 0x7001A, ov, set, sizeof set) == 0 && ov[0] == 87 && apc_count == 0);
  CHECK(KeDelayExecutionThread(1, 1, &zero) == 0xC0 && apc_count == 3 && apc_log[0] == 87 &&
        apc_log[1] == ov[1] && apc_log[2] == (u32)(uintptr_t)ov);
}

// XINPUT_STATE.
typedef struct {
  u32 packet;
  uint16_t buttons;
  uint8_t left_trigger, right_trigger;
  int16_t lx, ly, rx, ry;
} PadState;

// Reads player 1's pad until `buttons` are down (the pads are read every
// few milliseconds); its result.
static u32 PadWait(PadState* s, uint16_t buttons) {
  int64_t ms = -10000;
  u32 r = 0;
  for (int k = 0; k < 1000; k++) {
    r = XamInputGetState(0, 0, s);
    if (r || (s->buttons & buttons) == buttons) break;
    KeDelayExecutionThread(1, 0, &ms);
  }
  return r;
}

// Rumbles player 1's pad at `speed`, a request to the test program
// (title_main.cpp), and waits for XN_SYS_INPUTDEVICESCHANGED on h.
static int PadChange(u32 h, uint16_t speed, int notified) {
  uint16_t vibration[2] = {speed, 0};
  int64_t second = -10000000;
  u32 id = 0, value = 9;
  if (XamInputSetState(0, 0, vibration) != 0) return 0;
  if (!notified) return 1;
  return NtWaitForSingleObjectEx(h, 1, 0, &second) == 0 && XNotifyGetNext(h, 0x12, &id, &value) == 1 &&
         id == 0x12 && value == 0 && XNotifyGetNext(h, 0, &id, &value) == 0;
}

// Two pads (title_main.cpp): player 1's holds A, d-pad up, the guide
// button and the left stick up; player 2's nothing.
// ---- xam's network stack, with no cable --------------------------------------
struct SockAddr {
  uint16_t family, port;
  u32 addr;
  uint8_t zero[8];
};
static struct SockAddr Addr(u32 addr, uint16_t port) {
  struct SockAddr a = {2, port, addr, {0}};
  return a;
}
static u32 NetError(u32 result) { return result == 0xFFFFFFFF ? NetDll_WSAGetLastError() : 0; }

static void Net(void) {
  // inet_addr: decimal, octal and hex parts, short forms, failures.
  CHECK(NetDll_inet_addr("1.2.3.4") == 0x01020304 && NetDll_inet_addr("0x7f.1") == 0x7F000001 &&
        NetDll_inet_addr("010.0.0.1") == 0x08000001 && NetDll_inet_addr(" 10 ") == 10 && NetDll_inet_addr("") == 0);
  CHECK(NetDll_inet_addr("1.2.3.4.5") == 0xFFFFFFFF && NetDll_inet_addr("1..2") == 0xFFFFFFFF &&
        NetDll_inet_addr("0x") == 0xFFFFFFFF && NetDll_inet_addr("1.a") == 0xFFFFFFFF);
  CHECK(NetError(NetDll_socket(2, 2, 2, 17)) == 10093 && NetError(NetDll_WSACleanup(3)) == 10093 &&
        NetDll_XNetCleanup(3) == 10093);
  CHECK(NetError(NetDll_socket(1, 3, 2, 17)) == 10047 && NetError(NetDll_socket(1, 2, 5, 0)) == 10044 &&
        NetError(NetDll_socket(1, 2, 2, 6)) == 10043 && NetError(NetDll_socket(1, 2, 1, 17)) == 10043);

  // UDP to this console's own sockets.
  u32 a = NetDll_socket(1, 2, 2, 17), b = NetDll_socket(1, 0, 0, 17), v = NetDll_socket(1, 2, 0, 254);
  CHECK(a >> 28 == 1 && b >> 28 == 1 && v >> 28 == 1 && a != b && (a & 0xFFF) == 0 && (b & 0xFFF) == 1);
  struct SockAddr any = Addr(0, 5000), name;
  u32 len = 0, n = 0, opt = 0;
  CHECK(NetDll_bind(1, b, &any, 16) == 0 && NetError(NetDll_bind(1, b, &any, 16)) == 10022);
  CHECK(NetDll_getsockname(1, b, &name, &len) == 0 && len == 16 && name.family == 2 && name.port == 5000);
  CHECK(NetError(NetDll_getsockname(1, a, &name, &len)) == 10022);
  struct SockAddr there = Addr(0x0A000001, 5000), reserved = Addr(0, 3074);
  CHECK(NetError(NetDll_bind(1, a, &there, 16)) == 10049 && NetError(NetDll_bind(1, a, &reserved, 16)) == 10048 &&
        NetError(NetDll_bind(1, a, &any, 16)) == 10048);
  struct SockAddr loop = Addr(0x7F000001, 5000), none = Addr(0, 5000), other_loop = Addr(0x7F000002, 5000),
                  bcast = Addr(0xFFFFFFFF, 5000), far = Addr(0x01020304, 80), sa = Addr(0x00010203, 80);
  CHECK(NetDll_sendto(1, a, "hello", 5, 0, &loop, 16) == 5);
  CHECK(NetDll_getsockname(1, a, &name, &len) == 0 && name.port == 1025);  // bound on the way
  CHECK(NetDll_ioctlsocket(1, b, 0x4004667F, &n) == 0 && n == 5);
  char buf[16] = {0};
  struct SockAddr from;
  len = 3;
  CHECK(NetDll_recvfrom(1, b, buf, sizeof buf, 0, &from, &len) == 5 && Memeq(buf, "hello", 5) && len == 16 &&
        from.family == 2 && from.port == 1025 && from.addr == 0x7F000001);
  n = 1;
  CHECK(NetDll_ioctlsocket(1, b, 0x8004667E, &n) == 0 && NetError(NetDll_recv(1, b, buf, 16, 0)) == 10035);
  n = 0;
  opt = 50;
  CHECK(NetDll_ioctlsocket(1, b, 0x8004667E, &n) == 0 && NetDll_setsockopt(1, b, 0xFFFF, 0x1006, &opt, 4) == 0);
  CHECK(NetError(NetDll_recv(1, b, buf, 16, 0)) == 10060);  // SO_RCVTIMEO
  CHECK(NetDll_sendto(1, a, "world", 5, 0, &loop, 16) == 5 && NetError(NetDll_recv(1, b, buf, 2, 0)) == 10040 &&
        buf[0] == 'w' && buf[1] == 'o' && buf[2] == 'l');  // cut short and gone
  CHECK(NetError(NetDll_sendto(1, a, "x", 1, 0, &none, 16)) == 10049 &&
        NetError(NetDll_sendto(1, a, "x", 1, 0, &other_loop, 16)) == 10049 &&
        NetError(NetDll_sendto(1, a, "x", 1, 0, &bcast, 16)) == 10013 &&
        NetError(NetDll_sendto(1, a, "x", 1, 0, &sa, 16)) == 10065 &&
        NetDll_sendto(1, a, "x", 1, 0, &far, 16) == 1);  // lost: no link
  opt = 1;
  CHECK(NetDll_setsockopt(1, a, 0xFFFF, 0x20, &opt, 4) == 0 && NetDll_sendto(1, a, "x", 1, 0, &bcast, 16) == 1);
  CHECK(NetError(NetDll_send(1, a, "x", 1, 0)) == 10057 && NetDll_connect(1, a, &loop, 16) == 0 &&
        NetDll_send(1, a, "abc", 3, 0) == 3 && NetDll_recv(1, b, buf, 16, 0) == 3);
  CHECK(NetError(NetDll_sendto(1, v, "x", 1, 0, &loop, 16)) == 10040 && NetDll_sendto(1, v, "\0\1x", 3, 0, &loop, 16) == 3);
  u32 big = 0;
  CHECK(XamAlloc(0, 0x8001, &big) == 0 &&
        NetError(NetDll_sendto(1, a, (void*)(uintptr_t)big, 0x8001, 0, &far, 16)) == 10040);
  XamFree(big);
  // Options: their values, xam's limits and refusals.
  len = 4;
  CHECK(NetDll_getsockopt(1, a, 0xFFFF, 0x1008, &opt, &len) == 0 && opt == 2 && len == 4);
  CHECK(NetDll_getsockopt(1, a, 0xFFFF, 0x1002, &opt, &len) == 0 && opt == 0x4000);
  CHECK(NetDll_getsockopt(1, a, 0, 7, &opt, &len) == 0 && opt == 64);
  uint8_t byte = 0xAA;
  len = 1;
  CHECK(NetDll_getsockopt(1, a, 0, 7, &byte, &len) == 0 && byte == 64 && len == 1);
  len = 4;
  CHECK(NetError(NetDll_getsockopt(1, a, 6, 1, &opt, &len)) == 10042 &&
        NetError(NetDll_getsockopt(1, a, 99, 1, &opt, &len)) == 10022);
  opt = 1 << 20;
  CHECK(NetDll_setsockopt(1, a, 0xFFFF, 0x1001, &opt, 4) == 0 && NetDll_setsockopt(1, a, 0xFFFF, 0x1002, &opt, 4) == 0);
  CHECK(NetDll_getsockopt(1, a, 0xFFFF, 0x1001, &opt, &len) == 0 && opt == 0xFC00 &&
        NetDll_getsockopt(1, a, 0xFFFF, 0x1002, &opt, &len) == 0 && opt == 0xFC00);
  opt = 1;
  CHECK(NetDll_setsockopt(1, a, 0xFFFF, 4, &opt, 4) == 0 && NetError(NetDll_setsockopt(1, a, 0xFFFF, 4, &opt, 4)) == 10022);
  CHECK(NetError(NetDll_setsockopt(1, a, 0xFFFF, 0x5801, &opt, 4)) == 10013 &&  // no privilege
        NetError(NetDll_setsockopt(1, a, 0xFFFF, 0x5803, &opt, 4)) == 10050);  // no Live
  opt = 0;
  CHECK(NetError(NetDll_setsockopt(1, a, 0, 7, &opt, 4)) == 10022 && NetError(NetDll_setsockopt(1, v, 0xFFFF, 0x20, &opt, 4)) == 10042);
  u32 group = 0xE0000001;
  CHECK(NetDll_setsockopt(1, a, 0, 5, &group, 8) == 0 && NetError(NetDll_setsockopt(1, a, 0, 5, &group, 8)) == 10022 &&
        NetDll_setsockopt(1, a, 0, 6, &group, 8) == 0 && NetError(NetDll_setsockopt(1, a, 0, 6, &group, 8)) == 10022);
  CHECK(NetError(NetDll_ioctlsocket(1, a, 0x1234, &n)) == 10042);

  // TCP: refused by this console, otherwise no answer.
  u32 t = NetDll_socket(1, 2, 1, 0);
  uint16_t linger[2];
  len = 4;
  CHECK(NetDll_getsockopt(1, t, 0xFFFF, 0x80, linger, &len) == 0 && linger[0] == 0 && linger[1] == 60);
  CHECK(NetDll_getsockopt(1, t, 0xFFFF, ~0x80u, &opt, &len) == 0 && opt == 1 &&
        NetDll_getsockopt(1, t, 6, 0x5803, &opt, &len) == 0 && opt == 0x200);
  CHECK(NetError(NetDll_connect(1, t, &none, 16)) == 10049 && NetError(NetDll_connect(1, t, &loop, 16)) == 10061);
  CHECK(NetError(NetDll_recv(1, t, buf, 16, 0)) == 10057 && NetError(NetDll_send(1, t, "x", 1, 0)) == 10057);
  n = 1;
  CHECK(NetDll_ioctlsocket(1, t, 0x8004667E, &n) == 0 && NetError(NetDll_connect(1, t, &far, 16)) == 10035 &&
        NetError(NetDll_connect(1, t, &far, 16)) == 10037);
  CHECK(NetDll_closesocket(1, t) == 0 && NetError(NetDll_closesocket(1, t)) == 10038 &&
        NetError(NetDll_closesocket(1, 0x10000FFF)) == 10038);

  // The system's stack with its own settings: one datagram socket, a
  // 1 s connect timeout, 1 s QoS probes with 1 retry.
  uint8_t params[0x4C] = {0x4C};
  params[2] = 1;
  params[9] = 1;
  params[10] = 1;
  params[0x45] = 1;
  uint16_t wsa[0xC8];
  CHECK(NetDll_XNetStartup(2, params) == 0 && NetDll_WSAStartup(2, 0x0202, wsa) == 0);
  u32 d = NetDll_socket(2, 2, 2, 17), s2 = NetDll_socket(2, 2, 1, 6);
  CHECK(d >> 28 == 2 && NetError(NetDll_socket(2, 2, 2, 17)) == 10055);
  CHECK(NetError(NetDll_connect(2, s2, &far, 16)) == 10060);  // after 1 s
  CHECK(NetError(NetDll_closesocket(1, d)) == 10038);

  // XNet: this console's address, keys, security associations.
  uint8_t own[0x24], peer[0x24] = {0}, id[8], key[16];
  CHECK(NetDll_XNetGetTitleXnAddr(1, own) == 0x8002 && own[10] == 0x00 && own[11] == 0x22 && own[12] == 0x48 &&
        own[0x20] == 1 && *(u32*)own == 0);
  CHECK(NetDll_XNetGetDebugXnAddr(1, peer) == 1 && NetDll_XNetRandom(1, buf, 16) == 0);
  CHECK(NetDll_XNetCreateKey(1, id, key) == 0 && (id[0] & 0xF0) == 0);
  CHECK(NetDll_XNetRegisterKey(1, id, key) == 0 && NetDll_XNetRegisterKey(1, id, key) == 10037);
  u32 ina = 1, ina2 = 0;
  CHECK(NetDll_XNetXnAddrToInAddr(1, own, id, &ina) == 0 && ina == 0x7F000001);
  CHECK(NetDll_XNetXnAddrToInAddr(1, peer, id, &ina) == 10022 && ina == 0);  // no Ethernet address
  for (int k = 0; k < 6; k++) peer[10 + k] = (uint8_t)(k + 1);
  CHECK(NetDll_XNetXnAddrToInAddr(1, peer, id, &ina) == 0 && ina && ina >> 24 == 0 &&
        NetDll_XNetXnAddrToInAddr(1, peer, id, &ina2) == 0 && ina2 == ina);
  sa.addr = ina;
  CHECK(NetDll_sendto(1, a, "x", 1, 0, &sa, 16) == 1);  // through the association, lost
  uint8_t other[8] = {0x80, 1, 2, 3, 4, 5, 6, 7}, unknown[8] = {0, 9, 9, 9, 9, 9, 9, 9};
  CHECK(NetDll_XNetXnAddrToInAddr(1, peer, unknown, &ina) == 10022);
  *(u32*)(peer + 4) = 0x0A000001;
  CHECK(NetDll_XNetXnAddrToInAddr(1, peer, other, &ina) == 10049);  // online: needs Live

  // QoS: this console answers at once, another console's probes run out.
  CHECK(NetDll_XNetQosListen(1, id, "data", 4, 0, 5) == 0 && NetDll_XNetQosListen(1, unknown, 0, 0, 0, 1) == 10022);
  peer[0x20] = 1;  // a console like this one: probed
  u32 event = NetDll_WSACreateEvent(), xnqos = 0;
  u32 xnaddrs[2] = {(u32)(uintptr_t)own, (u32)(uintptr_t)peer}, ids[2] = {(u32)(uintptr_t)id, (u32)(uintptr_t)id},
      keys[2] = {(u32)(uintptr_t)key, (u32)(uintptr_t)key};
  CHECK(event && NetDll_XNetQosLookup13(2, 2, xnaddrs, ids, keys, 0, 0, 0, 0, 0, 0, event, &xnqos) == 0 && xnqos);
  CHECK(*At(xnqos) == 2 && *At(xnqos + 4) == 1 && *(uint8_t*)(uintptr_t)(xnqos + 8) == 0xB &&
        *(uint16_t*)(uintptr_t)(xnqos + 8 + 6) == 4 && Memeq((void*)(uintptr_t)*At(xnqos + 8 + 8), "data", 4) &&
        *At(xnqos + 8 + 0x10) == 100000000);
  int64_t ten_ms = -100000, wait = -15000000;
  KeDelayExecutionThread(1, 0, &wait);
  CHECK(*At(xnqos + 4) == 1);  // probes for 2 s
  for (int k = 0; k < 200 && *At(xnqos + 4); k++) KeDelayExecutionThread(1, 0, &ten_ms);
  CHECK(*At(xnqos + 4) == 0);
  CHECK(*(uint8_t*)(uintptr_t)(xnqos + 8 + 0x18) == 1);  // complete, never contacted
  CHECK(NetDll_XNetQosRelease(1, xnqos) == 10022 && NetDll_XNetQosRelease(2, xnqos) == 0);

  // DNS: no server.
  u32 xndns = 0, dns_event = NetDll_WSACreateEvent();
  int64_t zero = 0;
  CHECK(NetDll_XNetDnsLookup(1, "", 0, &xndns) == 10014 && NetDll_XNetDnsLookup(1, ".a", 0, &xndns) == 10014);
  CHECK(NtWaitForSingleObjectEx(dns_event, 1, 0, &zero) == kStatusTimeout);
  CHECK(NetDll_XNetDnsLookup(1, "host.example", dns_event, &xndns) == 0 && *At(xndns) == 11001 &&
        *At(xndns + 4) == 0 && NtWaitForSingleObjectEx(dns_event, 1, 0, &zero) == 0);
  CHECK(NetDll_XNetDnsRelease(2, xndns) == 10022 && NetDll_XNetDnsRelease(1, xndns) == 0);

  CHECK(NetDll_XNetUnregisterKey(2, id) == 10022 && NetDll_XNetUnregisterKey(1, id) == 0 &&
        NetDll_XNetUnregisterKey(1, id) == 10022);
  // Cleanup: the system's sockets go with its Winsock, the stack with the
  // last caller.
  CHECK(NetDll_WSACleanup(2) == 0 && NetError(NetDll_closesocket(2, d)) == 10093);
  CHECK(NetDll_WSAStartup(2, 0x0202, wsa) == 0 && NetError(NetDll_closesocket(2, d)) == 10038 &&
        NetDll_WSACleanup(2) == 0 && NetDll_XNetCleanup(2) == 0 && NetDll_XNetCleanup(2) == 10093);
  CHECK(NetDll_WSACleanup(1) == 0 && NetDll_WSACleanup(1) == 0 && NetDll_XNetCleanup(1) == 0 &&
        NetDll_XNetGetTitleXnAddr(1, own) == 0x8002 && NetDll_XNetCleanup(1) == 0);
  CHECK(NetDll_XNetGetTitleXnAddr(1, own) == 1 && NetDll_XNetRandom(1, buf, 4) == 10093);
  CHECK(NtClose(event) == 0 && NtClose(dns_event) == 0);
}

// ---- structured exception handling (seh.S) ------------------------------
static void Exceptions(void) {
  static uint8_t ctx[0xA40] __attribute__((aligned(16)));
  // The registers at the call, Iar and LR its return address, r3 kept.
  CHECK(capture(ctx) == 1);
  const volatile u32* w = (const volatile u32*)ctx;
  CHECK(w[0] == 7);                                // ContextFlags
  CHECK(w[2] == seh_log[9] && w[3] == seh_log[9]);  // Iar, Lr
  CHECK(w[(0x18 + 8 * 1 + 4) / 4] == seh_log[8]);   // r1
  CHECK(w[(0x18 + 8 * 3 + 4) / 4] == (u32)(uintptr_t)ctx);
  CHECK(w[(0x18 + 8 * 7 + 4) / 4] == 0x77770000);

  // A filter that takes the exception: its __except block, in its frame,
  // with r3 the code; the __finally further out then runs normally.
  for (int k = 0; k < 16; k++) seh_log[k] = 0;
  CHECK(seh_outer(0xE0000001) == 0x101);
  CHECK(seh_log[0] == seh_log[4]);  // the filter's frame: seh_try's
  // seh_outer's frame: seh_try's, 0x80 up.
  CHECK(seh_log[1] == 1 && seh_log[2] == 0 && seh_log[3] == seh_log[4] + 0x80);
  // One that searches on: the outer __except, after the __finally ran as
  // an abnormal termination.
  for (int k = 0; k < 16; k++) seh_log[k] = 0;
  CHECK(seh_outer(0xE0000002) == 0x202);
  CHECK(seh_log[0] == seh_log[4]);
  CHECK(seh_log[1] == 1 && seh_log[2] == 1 && seh_log[3] == seh_log[4] + 0x80);
  // One that continues execution: the raise returns.
  for (int k = 0; k < 16; k++) seh_log[k] = 0;
  CHECK(seh_outer(0xE0000003) == 7);
  CHECK(seh_log[1] == 1 && seh_log[2] == 0);
  // A title's own handler unwinding to the code after the call (its
  // handler is called again for the unwind's target frame).
  for (int k = 0; k < 16; k++) seh_log[k] = 0;
  CHECK(own_frame(0xE0000004) == 0x377);
  CHECK(seh_log[5] == 2);
  // An unwind whose target frame's handler puts back a context captured
  // further down: it returns where that context was captured.
  for (int k = 0; k < 16; k++) seh_log[k] = 0;
  CHECK(back_outer() == 0x444);
  CHECK(seh_log[6] == 1);

  // longjmp: a blr to a frame further up, through a frame in between.
  CHECK(sj_caller() == 0x505);

  // The L2 window is guest memory.
  CHECK(KeLockL2(0, 0x7F100000, 0x20000, 1, 1) == 0);
  *At(0x7F100000) = 0x12345678;
  CHECK(*At(0x7F100000) == 0x12345678);
  CHECK(KeUnlockL2(0) == 0);
}

static void Controllers(void) {
  uint8_t caps[0x14];
  memset(caps, 0xAA, sizeof caps);
  CHECK(XamInputGetCapabilities(0, 0, caps) == 0 && caps[0] == 1 && caps[1] == 1 && caps[2] == 0 &&
        caps[3] == 0 && caps[4] == 0xF3 && caps[5] == 0xFF && caps[0x13] == 0xFF);
  CHECK(XamInputGetCapabilities(1, 1, caps) == 0 && caps[0] == 1);
  // No keyboard, no type 3 context; flags naming no device fail.
  memset(caps, 0xAA, sizeof caps);
  CHECK(XamInputGetCapabilities(0, 2, caps) == 1167 && caps[0] == 0 && caps[0x13] == 0);
  CHECK(XamInputGetCapabilities(0, 9, caps) == 1167 && XamInputGetCapabilities(0, 0x10, caps) == 1167);
  CHECK(XamInputGetCapabilities(0, 0x40, caps) == 1627 && XamInputGetCapabilities(2, 1, caps) == 1167);
  CHECK(XamInputGetCapabilities(0xFF, 1, caps) == 1167);

  // The guide button stays clear; the stick's up is positive. The packet
  // number holds while the state does.
  PadState s, t;
  CHECK(PadWait(&s, 0x1001) == 0 && s.buttons == 0x1001 && s.left_trigger == 0 && s.right_trigger == 0 &&
        s.lx == 0 && s.ly == 32767 && s.rx == 0 && s.ry == 0 && s.packet != 0);
  CHECK(XamInputGetState(0, 0, &t) == 0 && t.packet == s.packet);
  CHECK(XamInputGetState(1, 0, &t) == 0 && t.buttons == 0 && XamInputGetState(0, 8, &t) == 1167);
  CHECK(XamInputGetState(0xFF, 0, &t) == 1167);

  // Player 2's pad unplugged and plugged in again: a notification each
  // time, and it is player 2's again; player 1 keeps theirs.
  u32 h = XamNotifyCreateListener(1, 1), id, value;
  while (XNotifyGetNext(h, 0, &id, &value)) {
  }
  CHECK(PadChange(h, 0x1111, 1));
  CHECK(XamInputGetState(1, 0, &t) == 1167 && XamInputGetCapabilities(1, 1, caps) == 1167);
  CHECK(XamInputGetState(0, 0, &t) == 0 && t.buttons == 0x1001);
  CHECK(PadChange(h, 0x2222, 1));
  CHECK(XamInputGetState(1, 0, &t) == 0 && XamInputGetState(2, 0, &t) == 1167);
  // B pressed, the left trigger pulled: a new packet.
  CHECK(PadChange(h, 0x3333, 0));
  CHECK(PadWait(&t, 0x3001) == 0 && t.buttons == 0x3001 && t.packet > s.packet);
  int64_t ms = -10000;
  for (int k = 0; k < 1000 && t.left_trigger != 255; k++) {
    KeDelayExecutionThread(1, 0, &ms);
    XamInputGetState(0, 0, &t);
  }
  CHECK(t.left_trigger == 255 && t.ly == 32767);
  NtClose(h);
}

int main(void) {
  Modules();
  Xam();
  Net();
  Exceptions();
  Controllers();
  Users();
  Content();
  Timers();
  Apcs();
  Sessions();
  Console();
  Gpu();
  Conversions();
  Audio();
  Xma();
  XmaDecode();
  Memory();
  Physical();
  Threads();
  Objects();
  Pulses();
  Handoff();
  Owners();
  Closes();
  Strings();
  Variables();
  Files();
  DbgPrint("runtime test: %d failed\n", failures);
  return failures;
}

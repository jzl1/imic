#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

typedef long long int64;

struct CPUInformation {
    int m_Size;
    unsigned char m_Flags;
    unsigned char m_nLogicalProcessors;
    unsigned char m_nPhysicalProcessors;
    int64 m_Speed;
    char *m_szProcessorID;
};

static struct CPUInformation CPUI;
static char vendor[] = "ValveFakeCPU";

static void mark(void) {
    int fd = open("/tmp/fakecpu_called", O_CREAT|O_WRONLY|O_APPEND, 0644);
    if (fd >= 0) { write(fd, "x", 1); close(fd); }
}

struct CPUInformation *GetCPUInformation(void) {
    mark();
    memset(&CPUI, 0, sizeof(CPUI));
    CPUI.m_Size = 20;
    CPUI.m_Flags = 0x5f;
    CPUI.m_nLogicalProcessors = 1;
    CPUI.m_nPhysicalProcessors = 1;
    CPUI.m_Speed = 1000000000LL;
    CPUI.m_szProcessorID = vendor;
    return &CPUI;
}

const char *GetProcessorVendorId(void) __asm__("_Z20GetProcessorVendorIdv");
const char *GetProcessorVendorId(void) {
    mark();
    return vendor;
}

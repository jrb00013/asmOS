#ifndef PS2_HARDWARE_H
#define PS2_HARDWARE_H

#include <stdint.h>
#include <stddef.h>

// PS2 Hardware Constants
#define PS2_MEMORY_SIZE_MB        32
#define PS2_MEMORY_SIZE_KB        (PS2_MEMORY_SIZE_MB * 1024)
#define PS2_MEMORY_SIZE_BYTES     (PS2_MEMORY_SIZE_KB * 1024)

// PS2 Memory Layout
#define PS2_KERNEL_BASE           0x10000
#define PS2_KERNEL_SIZE           0x4000  // 16KB
#define PS2_USER_SPACE_BASE       0x20000
#define PS2_STACK_BASE            0x80000

// PS2 Hardware Detection
#define PS2_SIGNATURE            0x12345678
#define PS2_MEMORY_SIGNATURE     'PAMS'

// PS2 Controller Ports
#define PS2_CONTROLLER1_PORT     0x60
#define PS2_CONTROLLER2_PORT     0x64
#define PS2_STATUS_PORT          0x64

// PS2 Controller Commands
#define PS2_CMD_DISABLE_PORT1    0xAD
#define PS2_CMD_ENABLE_PORT1     0xAE
#define PS2_CMD_DISABLE_PORT2    0xA7
#define PS2_CMD_ENABLE_PORT2     0xA8
#define PS2_CMD_READ_CONFIG      0x20
#define PS2_CMD_WRITE_CONFIG     0x60

// PS2 Hardware Information Structure
typedef struct {
    uint32_t total_memory_mb;
    uint8_t ps2_detected;
    uint8_t dualshock_support;
    uint8_t network_support;
    uint8_t cd_drive_present;
    uint32_t cpu_speed_mhz;
    char model_string[32];
} ps2_hardware_info_t;

// PS2 Memory Map Entry
typedef struct {
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attr;
} ps2_memory_map_entry_t;

// PS2 System Information
typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
} ps2_system_time_t;

// Function Declarations

// Hardware Detection
uint32_t detect_ps2_memory(void);
void init_ps2_controllers(void);
int detect_ps2_hardware(void);
void get_ps2_hardware_info(ps2_hardware_info_t* info);

// Memory Management
void* ps2_malloc(size_t size);
void ps2_free(void* ptr);
uint32_t get_ps2_memory_usage(void);

// System Functions
void ps2_reboot(void);
void ps2_shutdown(void);
ps2_system_time_t get_ps2_time(void);

// Controller Functions
int init_ps2_controller(int port);
int read_ps2_controller(int port, uint8_t* data);
int write_ps2_controller(int port, uint8_t data);

// CD/DVD Functions
int init_ps2_cd_drive(void);
int read_ps2_cd_sector(uint32_t lba, void* buffer);
int get_ps2_cd_status(void);

// Network Functions (if available)
int init_ps2_network(void);
int ps2_network_send(const void* data, size_t length);
int ps2_network_receive(void* buffer, size_t max_length);

// Utility Functions
void ps2_delay_ms(uint32_t milliseconds);
void ps2_delay_us(uint32_t microseconds);
uint32_t ps2_get_tick_count(void);

// PS2-specific error codes
#define PS2_SUCCESS              0
#define PS2_ERROR_MEMORY        -1
#define PS2_ERROR_CONTROLLER    -2
#define PS2_ERROR_CD_DRIVE      -3
#define PS2_ERROR_NETWORK       -4
#define PS2_ERROR_TIMEOUT       -5
#define PS2_ERROR_NOT_SUPPORTED -6

// PS2 Hardware Flags
#define PS2_FLAG_DUALSHOCK       0x01
#define PS2_FLAG_NETWORK         0x02
#define PS2_FLAG_CD_DRIVE        0x04
#define PS2_FLAG_USB_SUPPORT     0x08
#define PS2_FLAG_ETHERNET        0x10

#endif // PS2_HARDWARE_H

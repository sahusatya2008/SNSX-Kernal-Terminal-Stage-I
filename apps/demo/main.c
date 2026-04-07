#include "snsx_appabi.h"

int snsx_app_main(const SnsxAppApi *api) {
    api->writeln("[aurora-demo] SNSX ExOS application runtime is online.");
    api->writeln("[aurora-demo] Custom storage, ELF loading, and native apps are active.");
    api->write("[aurora-demo] Uptime ticks: ");
    api->write_u64(api->uptime_ticks());
    api->writeln("");
    api->write("[aurora-demo] Memory in use: ");
    api->write_u64(api->memory_used_bytes() / 1024u);
    api->writeln(" KiB");
    api->writeln("[aurora-demo] Next frontier: stronger isolation, GUI, networking, Wi-Fi, Bluetooth.");
    return 0;
}

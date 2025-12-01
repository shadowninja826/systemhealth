#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/statvfs.h>
#include <time.h>

void print_time() {
    printf("=== SYSTEM TIME ===\n");
    time_t t = time(NULL);
    struct tm *tm_info = localtime(&t);

    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", tm_info);

    printf("%s\n\n", buf);
}

/*
void check_cpu_thermals() {
    printf("=== CPU CORE TEMPERATURES (k10temp) ===\n");

    DIR *dir = opendir("/sys/class/hwmon");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        // skip non-directories
        if (entry->d_type != DT_LNK && entry->d_type != DT_DIR)
            continue;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        // protect against overly long names
        if (strlen(entry->d_name) > 100)
            continue;

        char hwmon_path[256];
        int written = snprintf(hwmon_path, sizeof(hwmon_path),
                               "/sys/class/hwmon/%s", entry->d_name);

        if (written <= 0 || written >= sizeof(hwmon_path))
            continue;  // would have overflowed; skip safely

        char name_file[300];
        snprintf(name_file, sizeof(name_file), "%s/name", hwmon_path);

        FILE *fp = fopen(name_file, "r");
        if (!fp)
            continue;

        char name[64];
        if (!fgets(name, sizeof(name), fp)) {
            fclose(fp);
            continue;
        }
        fclose(fp);

        name[strcspn(name, "\n")] = 0;

        if (strcmp(name, "k10temp") != 0)
            continue;

        printf("Found CPU sensor: %s (%s)\n", name, entry->d_name);

        // read all temp sensors available
        for (int i = 1; i <= 10; i++) {
            char temp_file[300];
            snprintf(temp_file, sizeof(temp_file), "%s/temp%d_input",
                     hwmon_path, i);

            fp = fopen(temp_file, "r");
            if (!fp)
                continue;

            int temp_mdeg;
            if (fscanf(fp, "%d", &temp_mdeg) == 1) {
                float tempC = temp_mdeg / 1000.0f;

                switch (i) {
                    case 1: printf("Tctl/Tdie: %.1f°C\n", tempC); break;
                    case 2: printf("CCD1: %.1f°C\n", tempC); break;
                    case 3: printf("CCD2: %.1f°C\n", tempC); break;
                    default:
                        printf("Temp%d: %.1f°C\n", i, tempC);
                        break;
                }
            }
            fclose(fp);
        }

        printf("\n");
        break; // done once k10temp is found
    }

    closedir(dir);
}
*/
void check_cpu_thermals() {
    printf("=== CPU CORE TEMPERATURES ===\n");

    DIR *dir = opendir("/sys/class/hwmon");
    if (!dir) {
        perror("opendir");
        return;
    }

    struct dirent *entry;

    while ((entry = readdir(dir)) != NULL) {

        if (entry->d_type != DT_LNK && entry->d_type != DT_DIR)
            continue;

        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char hwmon_path[256];
        snprintf(hwmon_path, sizeof(hwmon_path),
                 "/sys/class/hwmon/%s", entry->d_name);

        // Read the sensor name
        char name_file[300];
        snprintf(name_file, sizeof(name_file), "%s/name", hwmon_path);

        FILE *fp = fopen(name_file, "r");
        if (!fp)
            continue;

        char name[64];
        if (!fgets(name, sizeof(name), fp)) {
            fclose(fp);
            continue;
        }
        fclose(fp);

        name[strcspn(name, "\n")] = 0;

        // Accept Intel (coretemp) or AMD (k10temp)
        int is_amd = strcmp(name, "k10temp") == 0;
        int is_intel = strcmp(name, "coretemp") == 0;

        if (!is_amd && !is_intel)
            continue;

        printf("Found CPU sensor: %s (%s)\n\n", name, entry->d_name);

        // Loop temps
        for (int i = 1; i <= 20; i++) {
            char temp_file[300];
            snprintf(temp_file, sizeof(temp_file),
                     "%s/temp%d_input", hwmon_path, i);

            fp = fopen(temp_file, "r");
            if (!fp)
                continue;

            int temp_mdeg;
            if (fscanf(fp, "%d", &temp_mdeg) == 1) {
                float tempC = temp_mdeg / 1000.0f;

                if (is_amd) {
                    switch (i) {
                        case 1: printf("Tctl/Tdie: %.1f°C\n", tempC); break;
                        case 2: printf("CCD1: %.1f°C\n", tempC); break;
                        case 3: printf("CCD2: %.1f°C\n", tempC); break;
                        default: printf("Temp%d: %.1f°C\n", i, tempC);
                    }
                } else if (is_intel) {
                    printf("Core %d: %.1f°C\n", i - 1, tempC);
                }
            }

            fclose(fp);
        }

        printf("\n");
        break; // only print the first valid sensor
    }

    closedir(dir);
}

void check_disk_space() {
    printf("=== DISK SPACE UTILIZATION ===\n");

    struct statvfs info;

    if (statvfs("/", &info) == 0) {
        unsigned long total = info.f_blocks * info.f_frsize;
        unsigned long free = info.f_bfree * info.f_frsize;
        unsigned long used = total - free;

        printf("Total: %.2f GB\n", total / 1e9);
        printf("Used: %.2f GB\n", used / 1e9);
        printf("Free: %.2f GB\n", free / 1e9);
        printf("Usage: %.2f%%\n", (used * 100.0) / total);
    } else {
        perror("statvfs");
    }

    printf("\n");
}

void check_top_cpu_processes() {
    printf("=== TOP 5 CPU PROCESSES ===\n");

    FILE *fp = popen("ps -eo pid,comm,pcpu --sort=-pcpu | head -n 6", "r");
    if (!fp) {
        perror("popen");
        return;
    }

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), fp)) {
        printf("%s", buffer);
    }

    pclose(fp);
    printf("\n");
}

void check_memory_usage() {
    printf("=== MEMORY UTILIZATION ===\n");

    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("fopen");
        return;
    }

    char label[64];
    unsigned long value;
    unsigned long memtotal = 0, memfree = 0, buffers = 0, cached = 0;

    while (fscanf(fp, "%63s %lu kB", label, &value) != EOF) {
        if (strcmp(label, "MemTotal:") == 0) memtotal = value;
        else if (strcmp(label, "MemFree:") == 0) memfree = value;
        else if (strcmp(label, "Buffers:") == 0) buffers = value;
        else if (strcmp(label, "Cached:") == 0) cached = value;
    }

    fclose(fp);

    unsigned long used = memtotal - memfree - buffers - cached;

    printf("Total: %.2f GB\n", memtotal / 1e6);
    printf("Used: %.2f GB\n", used / 1e6);
    printf("Free: %.2f GB\n", memfree / 1e6);
    printf("Buffers/Cached: %.2f GB\n", (buffers + cached) / 1e6);
    printf("Usage: %.2f%%\n", (used * 100.0) / memtotal);

    printf("\n");
}

void read_last_7_lines() {
    printf("=== LAST 7 LINES OF /var/log/xmrig.log ===\n");

    FILE *fp = popen("tail -n 7 /var/log/xmrig.log 2>/dev/null", "r");
    if (!fp) {
        perror("popen");
        return;
    }

    char line[512];
    while (fgets(line, sizeof(line), fp)) {
        printf("%s", line);
    }

    pclose(fp);
    printf("\n");
}

int main() {
    print_time();
    check_cpu_thermals();
    check_disk_space();
    check_top_cpu_processes();
    check_memory_usage();
    read_last_7_lines();

    return 0;
}

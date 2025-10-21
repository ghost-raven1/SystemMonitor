#include "processes.h"
#include <libproc.h>
#include <sys/sysctl.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

size_t get_process_list(process_info_t *list, size_t max) {
    int pids[2048];
    int count = proc_listallpids(pids, sizeof(pids));
    if (count <= 0) return 0;

    size_t n = 0;
    for (int i = 0; i < count && n < max && i < 2048; i++) {
        int pid = pids[i];
        if (pid <= 0) continue;

        struct proc_bsdinfo info;
        if (proc_pidinfo(pid, PROC_PIDTBSDINFO, 0, &info, sizeof(info)) <= 0)
            continue;

        process_info_t *p = &list[n++];
        p->pid = pid;
        strncpy(p->name, info.pbi_name, sizeof(p->name) - 1);
        p->name[sizeof(p->name) - 1] = '\0';

        // CPU% based on delta of total user+system time between calls
        static struct {
            int pid;
            unsigned long long total_time_ns; // user+system
        } cache[4096];
        static size_t cache_len = 0;
        static struct timespec prev_ts = {0, 0};
        static int num_cpus = 0;

        if (num_cpus == 0) {
            int mibcpu[2] = {CTL_HW, HW_NCPU};
            size_t l = sizeof(num_cpus);
            sysctl(mibcpu, 2, &num_cpus, &l, NULL, 0);
            if (num_cpus <= 0) num_cpus = 1;
        }

        struct proc_taskinfo tinfo_cpu;
        float cpu_pct = 0.0f;
        if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &tinfo_cpu, sizeof(tinfo_cpu)) > 0) {
            unsigned long long total_ns = (unsigned long long)tinfo_cpu.pti_total_user + (unsigned long long)tinfo_cpu.pti_total_system;

            // find in cache
            size_t idx = cache_len;
            if (cache_len > 0) {
                for (size_t ci = 0; ci < cache_len && ci < 4096; ci++) {
                    if (cache[ci].pid == pid) { idx = ci; break; }
                }
            }

            struct timespec now;
            clock_gettime(CLOCK_MONOTONIC, &now);
            double dt = 0.0;
            if (prev_ts.tv_sec != 0 || prev_ts.tv_nsec != 0) {
                dt = (double)(now.tv_sec - prev_ts.tv_sec) + (double)(now.tv_nsec - prev_ts.tv_nsec)/1e9;
            }

            if (idx == cache_len && cache_len < 4096 && cache_len < (sizeof(cache)/sizeof(cache[0]))) {
                cache[cache_len].pid = pid;
                cache[cache_len].total_time_ns = total_ns;
                cache_len++;
            } else if (idx < cache_len && idx < 4096) {
                unsigned long long prev_ns = cache[idx].total_time_ns;
                unsigned long long delta_ns = (total_ns > prev_ns) ? (total_ns - prev_ns) : 0ULL;
                if (dt > 0.0 && num_cpus > 0) {
                    double cpu_frac = ((double)delta_ns / 1e9) / dt; // fraction of one CPU
                    cpu_pct = (float)(cpu_frac * 100.0 * (1.0));
                    // Normalize by number of CPUs to get 0-100% total across all cores
                    cpu_pct /= (float)num_cpus;
                }
                cache[idx].total_time_ns = total_ns;
            }

            prev_ts = now;
        }
        p->cpu_usage = cpu_pct;

        // MEM% по RSS
        struct proc_taskinfo tinfo;
        if (proc_pidinfo(pid, PROC_PIDTASKINFO, 0, &tinfo, sizeof(tinfo)) > 0) {
            long rss = tinfo.pti_resident_size;
            long total_mem;
            int mib[2] = {CTL_HW, HW_MEMSIZE};
            size_t len = sizeof(total_mem);
            sysctl(mib, 2, &total_mem, &len, NULL, 0);

            p->mem_usage = (float)rss / total_mem * 100.0f;
        } else {
            p->mem_usage = 0.0f;
        }
    }
    return n;
}

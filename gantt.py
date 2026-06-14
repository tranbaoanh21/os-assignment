import re
import sys
import os
import matplotlib.pyplot as plt

def parse_input(input_filename):
    num_cpus = 1
    try:
        with open(input_filename, 'r') as f:
            first_line = f.readline().strip()
            if first_line:
                parts = first_line.split()
                if len(parts) >= 2:
                    num_cpus = int(parts[1])
    except Exception as e:
        print(f"Lỗi đọc file input: {e}")
    return num_cpus


def parse_log(filename, num_cpus):
    cpu_state = {i: None for i in range(num_cpus)}
    cpu_intervals = {i: [] for i in range(num_cpus)}
    dispatch_time = {i: None for i in range(num_cpus)}
    current_time = None

    time_slot_pattern = re.compile(r"Time slot\s+(\d+)")
    dispatched_pattern = re.compile(r"CPU\s+(\d+):\s+Dispatched process\s+(\d+)")
    put_pattern = re.compile(r"CPU\s+(\d+):\s+Put process\s+(\d+)")
    processed_pattern = re.compile(r"CPU\s+(\d+):\s+Processed\s+(\d+)\s+has finished")
    stopped_pattern = re.compile(r"CPU\s+(\d+)\s+stopped")

    def close_interval(cpu, end_time):
        if cpu_state[cpu] is not None and dispatch_time[cpu] is not None:
            duration = end_time - dispatch_time[cpu]
            if duration > 0:
                cpu_intervals[cpu].append(
                    (cpu_state[cpu], dispatch_time[cpu], duration)
                )
        cpu_state[cpu] = None
        dispatch_time[cpu] = None

    with open(filename, 'r') as f:
        for line in f:
            line = line.strip()
            if not line:
                continue

            ts_match = time_slot_pattern.search(line)
            if ts_match:
                current_time = int(ts_match.group(1))
                continue

            d_match = dispatched_pattern.search(line)
            if d_match:
                cpu = int(d_match.group(1))
                proc = int(d_match.group(2))
                if cpu_state[cpu] is not None:
                    close_interval(cpu, current_time)
                cpu_state[cpu] = proc
                dispatch_time[cpu] = current_time
                continue

            put_match = put_pattern.search(line)
            if put_match:
                cpu = int(put_match.group(1))
                close_interval(cpu, current_time)
                continue

            p_match = processed_pattern.search(line)
            if p_match:
                cpu = int(p_match.group(1))
                close_interval(cpu, current_time)
                continue

            s_match = stopped_pattern.search(line)
            if s_match:
                cpu = int(s_match.group(1))
                close_interval(cpu, current_time)
                continue

    return cpu_intervals


def build_process_timeline(cpu_intervals):
    """
    Trả về:
    process_timeline[p] = list các interval (start, duration)
    """
    process_timeline = {}
    for intervals in cpu_intervals.values():
        for process, start, duration in intervals:
            process_timeline.setdefault(process, []).append((start, duration))

    # Merge adjacent dispatch intervals of the same process for a cleaner graph.
    for process, intervals in process_timeline.items():
        merged = []
        for start, duration in sorted(intervals):
            if merged and merged[-1][0] + merged[-1][1] == start:
                previous_start, previous_duration = merged[-1]
                merged[-1] = (previous_start, previous_duration + duration)
            else:
                merged.append((start, duration))
        process_timeline[process] = merged

    return process_timeline


def main():
    if len(sys.argv) < 4:
        print("python ganttchart.py <input> <log> <output.png>")
        sys.exit(1)

    input_file = sys.argv[1]
    log_file = sys.argv[2]
    out_img = sys.argv[3]

    num_cpus = parse_input(input_file)
    cpu_intervals = parse_log(log_file, num_cpus)
    process_timeline = build_process_timeline(cpu_intervals)
    processes = sorted(process_timeline.keys())

    # màu cho từng process
    colors = plt.cm.tab10.colors

    fig, ax = plt.subplots(figsize=(14, max(2, len(processes) * 0.6)))

    row_height = 0.5

    for i, p in enumerate(processes):
        y = i
        intervals = process_timeline[p]

        for (start, duration) in intervals:
            ax.broken_barh(
                [(start, duration)],
                (y, row_height),
                facecolors=colors[i % len(colors)],
                edgecolor='black',
                linewidth=0.5
            )

    # trục Y
    ax.set_yticks([i + row_height / 2 for i in range(len(processes))])
    ax.set_yticklabels([f"P{p}" for p in processes])

    # trục X (chỉ số, không grid nhỏ)
    max_time = max(
        max(start + duration for start, duration in intervals)
        for intervals in process_timeline.values()
    )

    ax.set_xticks(range(max_time))
    ax.set_xlim(0, max_time)
    for t in range(max_time):
        ax.axvline(
            x=t,
            color='gray',
            linestyle='--',
            linewidth=0.5,
            alpha=0.3,
            zorder=0  # nằm dưới block
        )
    ax.set_xlabel("Time Slot")
    input_name = os.path.basename(input_file)
    ax.set_title(f"{input_name} result")

    # bỏ grid rối mắt
    ax.grid(False)

    # bỏ viền thừa cho clean
    for spine in ["top", "right"]:
        ax.spines[spine].set_visible(False)

    plt.tight_layout()

    os.makedirs(os.path.dirname(out_img) if os.path.dirname(out_img) else '.', exist_ok=True)
    plt.savefig(out_img, dpi=150)

    print(f"[SUCCESS] Saved: {out_img}")


if __name__ == "__main__":
    main()

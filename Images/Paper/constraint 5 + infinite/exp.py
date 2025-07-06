import matplotlib.pyplot as plt
import matplotlib.patches as patches
import numpy as np
import seaborn as sns
from matplotlib.colors import LinearSegmentedColormap

# Set minimal style
plt.style.use('seaborn-v0_8-white')
sns.set_style("white")

# Data structure - using exactly the original data
cores = {
    "Core 0": [
        ("u1_0", 0, 1), ("u2_0", 1, 7), ("u3_0", 7, 9), ("u4_0", 9, 10),
        ("v8_0", 12, 13), ("u2_1", 13, 19), ("u3_1", 19, 21), ("u4_1", 21, 22),
        ("x4_1", 23, 24), ("u1_2", 24, 25), ("u2_2", 25, 31), ("u3_2", 31, 33),
        ("u4_2", 33, 34)
    ],
    "Core 1": [
        ("x1_0", 0, 1), ("x2_0", 1, 4), ("x4_0", 5, 6), ("v7_0", 7, 9),
        ("v4_0", 9, 12), ("u1_1", 12, 13), ("x1_1", 18, 19), ("x2_1", 19, 22)
    ],
    "Core 2": [
        ("v1_0", 0, 1), ("x3_0", 1, 5), ("v3_0", 9, 12), ("x3_1", 19, 23)
    ],
    "Core 3": [
        ("v6_0", 1, 2), ("v2_0", 2, 9)
    ],
    "Core 4": [
        ("v5_0", 1, 7)
    ]
}

# TDP data
tdp_values = [32.0, 75.0, 72.0, 72.0, 65.0, 58.0, 48.0, 37.0, 37.0, 31.0, 21.0, 21.0, 29.0, 
             14.0, 14.0, 14.0, 14.0, 14.0, 25.0, 34.0, 34.0, 34.0, 17.0, 10.0, 11.0, 
             14.0, 14.0, 14.0, 14.0, 14.0, 14.0, 10.0, 10.0, 10.0]

# Create custom colormaps with more shades
# Orange for dag01 (v tasks)
orange_colors = ['#FF8C00', '#FF7518', '#FF5E1E', '#FF4500', '#E83F0C', '#D13816', '#BA3120', '#A3292A']
orange_cmap = LinearSegmentedColormap.from_list('Orange_Shades', orange_colors)

# Blue for dag02 (x tasks)
blue_colors = ['#0047AB', '#0057B8', '#0067C5', '#0077D2', '#0087DF', '#0096EC', '#00A6F9', '#00B6FF']
blue_cmap = LinearSegmentedColormap.from_list('Blue_Shades', blue_colors)

# Green for dag03 (u tasks)
green_colors = ['#006400', '#007500', '#008600', '#009700', '#00A800', '#00B900', '#00CA00', '#00DB00']
green_cmap = LinearSegmentedColormap.from_list('Green_Shades', green_colors)

# Map task families to color maps
family_color_maps = {
    'v': orange_cmap,  # Orange for dag01
    'x': blue_cmap,    # Blue for dag02
    'u': green_cmap    # Green for dag03
}

# Map legend names
family_names = {
    'v': 'dag01 (v tasks)',
    'x': 'dag02 (x tasks)',
    'u': 'dag03 (u tasks)'
}

# Count and collect tasks by family
family_tasks = {'v': [], 'x': [], 'u': []}
for tasks in cores.values():
    for task, _, _ in tasks:
        family = task[0]
        if task not in family_tasks[family]:
            family_tasks[family].append(task)

# Create color map
task_color_map = {}
for family, tasks in family_tasks.items():
    cmap = family_color_maps[family]
    for i, task in enumerate(sorted(tasks)):
        # Normalize index to get value between 0 and 1
        idx = i / max(len(tasks) - 1, 1)
        task_color_map[task] = cmap(idx)

# Create figure with two subplots
fig, (ax_gantt, ax_tdp) = plt.subplots(2, 1, figsize=(20, 12), 
                                       gridspec_kw={'height_ratios': [3, 1]},
                                       sharex=True)
fig.patch.set_facecolor('white')
ax_gantt.set_facecolor('white')
ax_tdp.set_facecolor('white')

# Remove all gridlines
ax_gantt.grid(False)
ax_tdp.grid(False)

# Add only vertical lines to Gantt chart
for x in range(0, 36):
    ax_gantt.axvline(x=x, color='#e0e0e0', linestyle='-', linewidth=0.5, zorder=0)

# Plot tasks on Gantt chart
yticks = []
yticklabels = []
for i, (core, tasks) in enumerate(cores.items()):
    yticks.append(i)
    yticklabels.append(core)
    
    for task, start, end in tasks:
        duration = end - start
        color = task_color_map[task]
        
        # Create slightly rounded rectangle
        rect = patches.FancyBboxPatch(
            (start, i - 0.4), 
            duration, 
            0.8,
            boxstyle=patches.BoxStyle.Round(pad=0.02, rounding_size=0.05),
            facecolor=color, 
            edgecolor='black', 
            linewidth=1.0,
            alpha=1.0  # Full opacity for darker appearance
        )
        ax_gantt.add_patch(rect)
        
        # Add task label
        if duration >= 1:
            # Use white text for all backgrounds for consistency
            text_color = 'white'
            
            # For longer tasks, add a light border around text for better visibility
            if duration >= 2:
                # Add text shadow effect for better visibility
                for dx, dy in [(-0.5, -0.5), (-0.5, 0.5), (0.5, -0.5), (0.5, 0.5)]:
                    ax_gantt.text(
                        start + duration / 2 + dx*0.01, 
                        i + dy*0.01,
                        task, 
                        ha='center', 
                        va='center',
                        color='black', 
                        fontsize=9, 
                        fontweight='bold',
                        alpha=0.5,
                        zorder=4
                    )
                
            ax_gantt.text(
                start + duration / 2, 
                i,
                task, 
                ha='center', 
                va='center',
                color=text_color, 
                fontsize=9, 
                fontweight='bold',
                zorder=5
            )

# Create a legend for DAGs with darker colors
legend_elements = []
for family, name in family_names.items():
    cmap = family_color_maps[family]
    legend_elements.append(
        patches.Patch(facecolor=cmap(0.5), edgecolor='black', label=name)
    )

ax_gantt.legend(handles=legend_elements, loc='upper center', 
          bbox_to_anchor=(0.5, -0.05), ncol=3, frameon=True)

# Set up Gantt chart axes with darkened colors
ax_gantt.set_yticks(yticks)
ax_gantt.set_yticklabels(yticklabels, color='#333333', fontweight='bold')
#ax_gantt.set_title("Schedule when given infinite cores and high core TDP", 
            #color='#333333', fontweight='bold', pad=20)

# X-axis settings for Gantt chart
ax_gantt.set_xlim(0, 35)
ax_gantt.tick_params(axis='x', colors='#333333', labelcolor='#333333')

# Ensure all cores are visible with proper margins
ax_gantt.set_ylim(-0.8, len(cores) - 0.2)

# Add darkened axes for Gantt chart
for spine in ax_gantt.spines.values():
    spine.set_visible(True)
    spine.set_color('#333333')
    spine.set_linewidth(1.5)

# Plot TDP line chart in the bottom subplot
x_vals = np.arange(0.5, len(tdp_values) + 0.5)  # Center points at 0.5, 1.5, 2.5, etc.
ax_tdp.plot(x_vals, tdp_values, color='blue', linewidth=2, marker='o', markersize=4)

# Add horizontal line at TDP=50
ax_tdp.axhline(y=50, color='red', linestyle='-', linewidth=2, label='TDP Threshold (50)')

# Fill the area where TDP exceeds the threshold
ax_tdp.fill_between(x_vals, tdp_values, 50, where=(np.array(tdp_values) > 50), 
                   color='red', alpha=0.3)

# Set up TDP chart axes
ax_tdp.set_xlabel("Time", color='#333333', fontweight='bold')
ax_tdp.set_ylabel("Chip TDP", color='#333333', fontweight='bold')
ax_tdp.set_xlim(0, 35)
ax_tdp.set_xticks(np.arange(0, 36, 1))
ax_tdp.set_xticklabels([str(int(i)) for i in np.arange(0, 36, 1)], rotation=30)
ax_tdp.tick_params(axis='both', colors='#333333', labelcolor='#333333')
ax_tdp.grid(axis='both', linestyle='--', alpha=0.3)
ax_tdp.legend(loc='upper right')

# Add darkened axes for TDP chart
for spine in ax_tdp.spines.values():
    spine.set_visible(True)
    spine.set_color('#333333')
    spine.set_linewidth(1.5)

# Adjust layout
plt.tight_layout()
plt.subplots_adjust(hspace=0.1)  # Reduce space between subplots

plt.show()

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
        ("u1_1", 12, 13), ("u2_1", 13, 19), ("u3_1", 19, 21),
        ("u4_1", 21, 22), ("x4_1", 23, 24), ("u1_2", 24, 25),
        ("u2_2", 25, 31), ("u3_2", 31, 33), ("u4_2", 33, 34)
    ],
    "Core 1": [
        ("x1_0", 0, 1), ("x2_0", 1, 4), ("x4_0", 5, 6), ("v7_0", 7, 9),
        ("v8_0", 9, 10), ("x1_1", 18, 19), ("x2_1", 19, 22)
    ],
    "Core 2": [
        ("v1_0", 0, 1), ("x3_0", 1, 5), ("x3_1", 19, 23)
    ],
    "Core 3": [("v4_0", 1, 4)],
    "Core 4": [("v3_0", 1, 4)],
    "Core 5": [("v2_0", 1, 8)],
    "Core 6": [("v6_0", 1, 2)],
    "Core 7": [("v5_0", 1, 7)],
}

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

# Create figure with clean background
fig, ax = plt.subplots(figsize=(20, 8))
fig.patch.set_facecolor('white')
ax.set_facecolor('white')

# Remove all gridlines
ax.grid(False)

# Add only vertical lines
for x in range(0, 36):
    ax.axvline(x=x, color='#e0e0e0', linestyle='-', linewidth=0.5, zorder=0)

# Plot tasks
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
        ax.add_patch(rect)
        
        # Add task label
        if duration >= 1:
            # Use white text for all backgrounds for consistency
            text_color = 'white'
            
            # For longer tasks, add a light border around text for better visibility
            if duration >= 2:
                # Add text shadow effect for better visibility
                for dx, dy in [(-0.5, -0.5), (-0.5, 0.5), (0.5, -0.5), (0.5, 0.5)]:
                    ax.text(
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
                
            ax.text(
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

ax.legend(handles=legend_elements, loc='upper center', 
          bbox_to_anchor=(0.5, -0.1), ncol=3, frameon=True)

# Set up axes with darkened colors
ax.set_yticks(yticks)
ax.set_yticklabels(yticklabels, color='#333333', fontweight='bold')
ax.set_xlabel("Time", color='#333333', fontweight='bold')
ax.set_title("Schedule when given infinite cores and high core TDP", 
            color='#333333', fontweight='bold')

# X-axis settings
ax.set_xlim(0, 35)
ax.set_xticks(np.arange(0, 36, 1))
ax.tick_params(axis='x', colors='#333333', labelcolor='#333333')

# Ensure all cores are visible with proper margins
ax.set_ylim(-0.8, len(cores) - 0.2)

# Add darkened axes
for spine in ax.spines.values():
    spine.set_visible(True)
    spine.set_color('#333333')
    spine.set_linewidth(1.5)

# Adjust layout
plt.tight_layout()
plt.subplots_adjust(bottom=0.15)  # Make space for legend

plt.show()

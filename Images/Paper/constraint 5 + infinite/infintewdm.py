import matplotlib.pyplot as plt
import numpy as np

# Task data parsed from the provided information
time_data = list(range(34))  # 0 to 33
tasks_by_time = [
    ['u1_0', 'x1_0', 'v1_0'],                      # t=0
    ['u2_0', 'x2_0', 'x3_0', 'v6_0', 'v5_0'],       # t=1
    ['u2_0', 'x2_0', 'x3_0', 'v5_0', 'v2_0'],       # t=2
    ['u2_0', 'x2_0', 'x3_0', 'v5_0', 'v2_0'],       # t=3
    ['u2_0', 'x3_0', 'v5_0', 'v2_0'],               # t=4
    ['u2_0', 'x4_0', 'v5_0', 'v2_0'],               # t=5
    ['u2_0', 'v5_0', 'v2_0'],                       # t=6
    ['u3_0', 'v2_0', 'v7_0'],                       # t=7
    ['u3_0', 'v2_0', 'v7_0'],                       # t=8
    ['u4_0', 'v4_0', 'v3_0'],                       # t=9
    ['v4_0', 'v3_0'],                               # t=10
    ['v4_0', 'v3_0'],                               # t=11
    ['v8_0', 'u1_1'],                               # t=12
    ['u2_1'],                                       # t=13
    ['u2_1'],                                       # t=14
    ['u2_1'],                                       # t=15
    ['u2_1'],                                       # t=16
    ['u2_1'],                                       # t=17
    ['u2_1', 'x1_1'],                               # t=18
    ['u3_1', 'x2_1', 'x3_1'],                       # t=19
    ['u3_1', 'x2_1', 'x3_1'],                       # t=20
    ['u4_1', 'x2_1', 'x3_1'],                       # t=21
    ['x3_1'],                                       # t=22
    ['x4_1'],                                       # t=23
    ['u1_2'],                                       # t=24
    ['u2_2'],                                       # t=25
    ['u2_2'],                                       # t=26
    ['u2_2'],                                       # t=27
    ['u2_2'],                                       # t=28
    ['u2_2'],                                       # t=29
    ['u2_2'],                                       # t=30
    ['u3_2'],                                       # t=31
    ['u3_2'],                                       # t=32
    ['u4_2']                                        # t=33
]



# Create figure and axes
fig, ax = plt.subplots(figsize=(15, 8))

# Function to identify unique task combinations and merge consecutive time slots
def identify_task_blocks(tasks_by_time):
    blocks = []
    current_block = None
    
    for t, tasks in enumerate(tasks_by_time):
        # Sort tasks to ensure consistent comparison
        sorted_tasks = sorted(tasks)
        
        # If this is the first time slot or tasks changed
        if current_block is None or sorted_tasks != current_block['tasks']:
            if current_block is not None:
                blocks.append(current_block)
            
            if sorted_tasks:  # Only create a new block if there are tasks
                current_block = {
                    'start': t,
                    'end': t + 1,
                    'tasks': sorted_tasks,
                    'height': len(sorted_tasks)
                }
            else:
                current_block = None
        else:
            # Extend the current block
            current_block['end'] = t + 1
    
    # Add the last block if it exists
    if current_block is not None:
        blocks.append(current_block)
    
    return blocks

# Get merged blocks
blocks = identify_task_blocks(tasks_by_time)

# Plot each block with monochrome shading
for block in blocks:
    width = block['end'] - block['start']
    height = block['height']
    
    # Plot the block with light gray fill
    ax.bar(block['start'], height, width=width, 
           color='lightgray', edgecolor='black', align='edge')
    
    # Add task labels inside the block with black text
    task_list = block['tasks']
    for i, task in enumerate(task_list):
        y_position = i + 0.5
        ax.text(block['start'] + width/2, y_position, task,
                ha='center', va='center', fontsize=10, color='black')

# Set axis labels and title
ax.set_xlabel('Time (t)', fontsize=12)
ax.set_ylabel('Height (h)', fontsize=12)
ax.set_title('(b) Parallel Distribution Model', fontsize=14)

# Set x-ticks
ax.set_xticks(range(0, 34, 2))
ax.set_xticklabels([str(i) for i in range(0, 34, 2)], fontsize=10)

# Set y-ticks
max_height = max(block['height'] for block in blocks) if blocks else 0
ax.set_yticks(range(0, max_height + 2))
ax.set_yticklabels([str(i) for i in range(0, max_height + 2)], fontsize=10)

# Turn off the grid
ax.grid(False)

# Add a vertical line at t=0
ax.axvline(x=0, color='black', linestyle='-')

# Add a horizontal line at h=0
ax.axhline(y=0, color='black', linestyle='-')

# Set axis limits
ax.set_xlim(0, 34)
ax.set_ylim(0, max_height + 1)

plt.tight_layout()
plt.show()

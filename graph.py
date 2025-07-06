import pandas as pd
import matplotlib.pyplot as plt
import numpy as np

# Read the CSV files
df_schedulability = pd.read_csv('plotting_success_rate.csv')  # Updated filename
df_makespan = pd.read_csv('plotting_makespan.csv')
df_app_set = pd.read_csv('plotting_app_set_schedulability.csv')  # NEW FILE

# Define colors and strategy labels
colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd']
strategies = ['Strategy0', 'Strategy1', 'Strategy2', 'Strategy3', 'Strategy4']
strategy_labels = ['Earliest Deadline First', 'FIFO', 'Least Laxity First', 'Max Avg Power First', 'Highest Urgency Factor']

# Function to create histogram with values on top
def create_histogram(df, strategies, strategy_labels, colors, title, ylabel, filename_prefix):
    # Increased figure size significantly for better visibility
    fig, ax = plt.subplots(figsize=(18, 10))
    
    # Get unique TDP values
    tdp_values = df['TDP'].values
    n_tdp = len(tdp_values)
    n_strategies = len(strategies)
    
    # Set width of bars and positions - increased spacing
    bar_width = 0.12
    x_positions = np.arange(n_tdp) * 1.2  # More spacing between TDP groups
    
    # Enhanced color palette with gradients
    enhanced_colors = ['#1f77b4', '#ff7f0e', '#2ca02c', '#d62728', '#9467bd']
    
    # Create bars for each strategy with enhanced styling
    for i, (strategy, label) in enumerate(zip(strategies, strategy_labels)):
        values = df[strategy].values
        positions = x_positions + (i - 2) * bar_width  # Center the bars around x_positions
        
        # Create gradient effect by adding subtle variation
        bars = ax.bar(positions, values, bar_width, 
                     color=enhanced_colors[i], alpha=0.85, 
                     label=label, edgecolor='white', linewidth=1.5,
                     zorder=3)  # Bring bars to front
        
        # Add value labels on top of bars with rotation and better positioning
        for j, (bar, value) in enumerate(zip(bars, values)):
            height = bar.get_height()
            # Increased spacing and rotation for better readability
            ax.text(bar.get_x() + bar.get_width()/2., height + height*0.03,
                   f'{value:.3f}', ha='center', va='bottom', 
                   fontsize=9, fontweight='bold', rotation=25,
                   bbox=dict(boxstyle='round,pad=0.2', facecolor='white', 
                            alpha=0.8, edgecolor='none'))
    
    # Enhanced plot customization
    ax.set_xlabel('TDP (Thermal Design Power)', fontsize=16, fontweight='bold', 
                  labelpad=15)
    ax.set_ylabel(ylabel, fontsize=16, fontweight='bold', labelpad=15)
    ax.set_title(title, fontsize=18, fontweight='bold', pad=30,
                bbox=dict(boxstyle='round,pad=0.5', facecolor='lightblue', alpha=0.3))
    
    # Set x-axis labels with better formatting
    ax.set_xticks(x_positions)
    ax.set_xticklabels([f'{int(tdp)}W' for tdp in tdp_values], fontsize=12, fontweight='bold')
    
    # Enhanced grid with multiple styles
    ax.grid(True, alpha=0.4, linestyle='-', linewidth=0.8, axis='y', zorder=1)
    ax.grid(True, alpha=0.2, linestyle='--', linewidth=0.5, axis='x', zorder=1)
    
    # Beautiful legend with enhanced styling
    legend = ax.legend(title='🎯 DAG Selection Strategies', title_fontsize=13, fontsize=11, 
              loc='upper left', frameon=True, shadow=True, 
              bbox_to_anchor=(1.02, 1), fancybox=True, framealpha=0.95,
              edgecolor='black', facecolor='white')
    legend.get_title().set_fontweight('bold')
    
    # Enhanced styling
    ax.tick_params(axis='both', which='major', labelsize=12, width=2, length=6)
    ax.tick_params(axis='both', which='minor', width=1, length=3)
    
    # Add subtle background color
    ax.set_facecolor('#fafafa')
    
    # Set spine colors and width
    for spine in ax.spines.values():
        spine.set_linewidth(2)
        spine.set_edgecolor('#333333')
    
    # Dynamic y-axis limits with padding
    y_max = df[strategies].max().max()
    y_min = df[strategies].min().min()
    y_padding = (y_max - y_min) * 0.15
    ax.set_ylim(max(0, y_min - y_padding), y_max + y_padding)
    
    # Adjust layout to prevent legend cutoff
    plt.tight_layout()
    plt.subplots_adjust(right=0.72)
    
    plt.show()

# Create histogram for success rate data (updated from schedulability)
print("Creating Schedulability Rate Histogram...")
create_histogram(df_schedulability, strategies, strategy_labels, colors,
                'DAG Scheduling Strategies: TDP vs Schedulability Rate (Histogram)',
                'Schedulability Rate', 'schedulability_rate')

# Create histogram for Application Set Schedulability data (NEW)
print("\nCreating Application Set Schedulability Histogram...")
create_histogram(df_app_set, strategies, strategy_labels, colors,
                'DAG Scheduling Strategies: TDP vs Application Set Schedulability (Histogram)',
                'Application Set Schedulability (%)', 'app_set_schedulability')

# Create histogram for makespan data
print("\nCreating Makespan Histogram...")
# Update strategy labels for makespan (Strategy1 has different description)
strategy_labels_makespan = ['Earliest Deadline First', 'FIFO', 'Least Laxity First', 'Max Avg Power First', 'Highest Urgency Factor']

create_histogram(df_makespan, strategies, strategy_labels_makespan, colors,
                'DAG Scheduling Strategies: TDP vs Makespan (Histogram)',
                'Makespan', 'makespan')

print("\nHistogram conversion complete!")
print("\nKey features of the new histograms:")
print("- 5 bars for each TDP value representing different strategies")
print("- Values displayed on top of each bar")
print("- Color-coded strategies with legend")
print("- Grid lines for better readability")
print("- Proper spacing and labels")
print("\nNew metrics added:")
print("- Success Rate: Continuous metric showing scheduling performance")
print("- Application Set Schedulability: Binary success metric (percentage of perfect schedules)")
print("- Makespan: Time-based performance metric")

import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import glob
import os
import matplotlib.gridspec as gridspec


def get_latest_stats_file(folder_path):
    # Search for all CSV files starting with 'analytics_'
    search_pattern = os.path.join(folder_path, "analytics_*.csv")
    files = glob.glob(search_pattern)
    
    if not files:
        return None
    
    # Sort files by creation time (the timestamp in the name also works)
    # this picks the newest file added to the folder
    latest_file = max(files, key=os.path.getctime)
    return latest_file


def run_analysis():
    stats_folder = "analytics" 
    file_path = get_latest_stats_file(stats_folder)
    
    if not file_path:
        print(f"Error: No analytics files found!")
        return

    # 1. Parse Metadata into a clean list
    params = []
    with open(file_path, 'r') as f:
        for line in f:
            if line.startswith('#') and "METADATA" not in line:
                params.append(line.replace('#', '').strip())
            elif line.startswith('Timestamp'):
                break

    # 2. Load Data
    df = pd.read_csv(file_path, comment='#')
    df['TimeSeconds'] = (df['Timestamp'] - df['Timestamp'].min()) / 1000.0

    # 3. Calculate Session Stats
    total_clicks = len(df)
    session_time = df['TimeSeconds'].max()
    avg_actual_cps = df['ActualCPS'].mean()
    avg_hold_ms = df['HoldMS'].mean()

    # --- Combine Stats + Metadata for the header ---
    session_stats = (
        f"Session Duration: {session_time:.1f}s | "
        f"Total Clicks: {total_clicks} | "
        f"Avg Speed: {avg_actual_cps:.2f} CPS | "
        f"Avg Hold: {avg_hold_ms:.1f}ms"
    )
    
    config_params = "\n".join(params)
    header_text = f"{session_stats}\n---\n{config_params}"

    # 4. Visualization Setup
    plt.style.use('seaborn-v0_8-darkgrid')
    
    # --- NEW: GRIDSPEC LAYOUT ---
    # Total Figure Size: Making it longer (height=14 instead of 11)
    fig = plt.figure(figsize=(16, 14)) 
    
    # Create a 3x2 grid with specified height ratios
    # Row 0: Header (20% of height)
    # Rows 1, 2: Plots (40% of height each)
    gs = gridspec.GridSpec(3, 2, height_ratios=[1, 3, 3], figure=fig)

    # A. Add Title
    fig.suptitle(f"GhostTap: {os.path.basename(file_path)}", 
                 fontsize=18, fontweight='bold', y=0.98)

    # B. Define Plotting Areas (Axes)
    ax0_0 = fig.add_subplot(gs[1, 0]) # Row 1, Col 0: CPS Plot
    ax0_1 = fig.add_subplot(gs[1, 1]) # Row 1, Col 1: Trend Plot
    ax1_0 = fig.add_subplot(gs[2, 0]) # Row 2, Col 0: Hold Plot
    ax1_1 = fig.add_subplot(gs[2, 1]) # Row 2, Col 1: Interval Plot

    # --- PLOTS ---
    # CPS Plot (Top Left)
    ax0_0.plot(df['TimeSeconds'], df['TargetCPS'], label='Target', alpha=0.3, color='blue')
    ax0_0.plot(df['TimeSeconds'], df['ActualCPS'], label='Actual', color='red', linewidth=1.5)
    ax0_0.set_title('CPS Performance')
    ax0_0.set_ylabel('CPS')
    ax0_0.legend(loc='upper right')

    # Trend Plot (Top Right)
    ax0_1.plot(df['TimeSeconds'], df['Trend'], color='purple', alpha=0.8)
    ax0_1.axhline(0, color='black', linestyle='--', alpha=0.3)
    ax0_1.set_title('Humanization Trend')
    ax0_1.set_ylabel('Trend Offset')

    # Hold Distribution (Bottom Left)
    sns.histplot(df['HoldMS'], kde=True, ax=ax1_0, color='green')
    ax1_0.set_title('Hold Duration')
    ax1_0.set_xlabel('Hold Duration (ms)')

    # Interval Distribution (Bottom Right)
    sns.histplot(df['IntervalMS'], kde=True, ax=ax1_1, color='orange')
    ax1_1.set_title('Interval Distribution')
    ax1_1.set_xlabel('Interval Duration (ms)')

    # --- THE TOP HEADER ---
    # Use a dummy text-only plot in the top-left slot to hold the data
    # We turn off the axis so it's just text
    header_ax = fig.add_subplot(gs[0, :]) # Use the ENTIRE top row
    header_ax.axis('off') # Hide grids, ticks, spines

    # Add the text directly to this axis, aligned left
    header_ax.text(0.01, 0.8, header_text, fontsize=12, family='monospace',
                  verticalalignment='center', wrap=True,
                  bbox=dict(facecolor='white', alpha=0.8, edgecolor='gray'))

    # Adjust layout to prevent overlap, leaving margin at the very bottom
    plt.tight_layout(rect=[0, 0.02, 1, 0.95])
    
    # 5. Save
    output_folder = "plots"
    if not os.path.exists(output_folder): os.makedirs(output_folder)
    output_name = os.path.basename(file_path).replace('.csv', '.png')
    plt.savefig(os.path.join(output_folder, output_name), dpi=300)
    
    print(f"Analysis Complete: {output_name}")
    plt.show()

if __name__ == "__main__":
    run_analysis()
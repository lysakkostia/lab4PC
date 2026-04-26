import pandas as pd
import matplotlib.pyplot as plt
import os

output_dir = 'pngresults'
if not os.path.exists(output_dir):
    os.makedirs(output_dir)

df = pd.read_csv('results.csv')

sizes = df['Size'].unique()

for size in sizes:
    subset = df[df['Size'] == size]

    plt.figure(figsize=(10, 6))


    threads_str = subset['Threads'].astype(str)


    plt.plot(threads_str, subset['Time_ms'], marker='o', linewidth=2, color='#ED7D31')

    plt.title(f'Matrix Size: {size}', fontsize=16, fontweight='bold')


    plt.xlabel('Number of Threads', fontsize=12)
    plt.ylabel('Execution Time (ms)', fontsize=12)

    plt.grid(True, linestyle='--', alpha=0.7)

    plt.tight_layout()

    filename = os.path.join(output_dir, f'plot_size_{size}.png')
    plt.savefig(filename, dpi=300)
    print(f"Plot successfully saved: {filename}")

    plt.close()

print("\nAll plots successfully generated in the 'pngresults' folder!")
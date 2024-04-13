import csv

# Update file paths accordingly
file_path_process_1 = 'perf_receiver_stats.csv'
file_path_process_2 = 'perf_sender_stats.csv'
output_file_path = 'combined_metrics.csv'

import csv

def combine_numeric_parts_of_row(row):
    numeric_parts = [part for part in row if part.isdigit()]
    return ''.join(numeric_parts)

def is_number(s):
    """Check if the string s is a number."""
    try:
        float(s.replace(',', ''))
        return True
    except ValueError:
        return False

def parse_numeric_value(row):
    # Attempt to parse the first item as an integer after removing commas.
    try:
        return int(row[0].replace(',', ''))
    except ValueError:
        return None

def read_and_parse_csv(file_path):
    metrics = []
    with open(file_path, newline='') as csvfile:
        reader = csv.reader(csvfile)
        for row in reader:
            numeric_value = parse_numeric_value(row)
            if numeric_value is not None:
                metrics.append((numeric_value, row))
    return metrics

def combine_metrics(metrics1, metrics2):
    combined = []
    for (value1, row1), (value2, row2) in zip(metrics1, metrics2):
        combined_value = int(combine_numeric_parts_of_row(row1)) + int(combine_numeric_parts_of_row(row2))

        first_non_numeric_index = 0
        for part in row1:
            try:
                float(part.replace(',', ''))
                first_non_numeric_index += 1
            except ValueError:
                break

        # Create a copy of the row starting from the first non-numeric index
        combined_row = [f"{combined_value:,}"] + row1[first_non_numeric_index:]
        combined.append(combined_row)
    return combined

try:
    metrics_process_1 = read_and_parse_csv(file_path_process_1)
    metrics_process_2 = read_and_parse_csv(file_path_process_2)
    combined_metrics = combine_metrics(metrics_process_1, metrics_process_2)

    with open(output_file_path, 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        for row in combined_metrics:
            writer.writerow(row)
    print(f"Combined metrics written to {output_file_path}")
except ValueError as e:
    print(e)
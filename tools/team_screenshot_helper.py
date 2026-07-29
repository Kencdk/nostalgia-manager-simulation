#!/usr/bin/env python3
"""
Team Screenshot Helper
Generates a mapping of Team IDs to Team Names from TeamsDB.csv
This helps you name your screenshot files correctly.
"""

import csv
import sys
from pathlib import Path

def generate_team_id_mapping(teamsdb_path):
    """Read TeamsDB.csv and print team ID to name mapping."""

    if not Path(teamsdb_path).exists():
        print(f"Error: {teamsdb_path} not found!")
        return

    print("=" * 70)
    print("TEAM ID MAPPING FOR SCREENSHOTS")
    print("=" * 70)
    print("Use these IDs to name your screenshot files:")
    print("Example: For 'Man Utd' with ID 42, name the file '42.png'\n")

    with open(teamsdb_path, 'r', encoding='utf-8') as f:
        reader = csv.DictReader(f, delimiter=';')
        team_id = 1  # IDs start at 1 in the C++ code

        for row in reader:
            club_name = row.get('Club', '').strip()
            if not club_name:
                continue

            league = row.get('League', 'Unknown').strip()

            # Print mapping
            print(f"ID {team_id:3d}: {club_name:40s} ({league})")
            team_id += 1

    print("\n" + "=" * 70)
    print(f"Total teams: {team_id - 1}")
    print("=" * 70)
    print("\nNow name your screenshots:")
    print("  1.png, 2.png, 3.png, ... etc.")
    print("Or add an 'ID' column to TeamsDB.csv with your preferred IDs")

def add_id_column_to_csv(teamsdb_path, output_path=None):
    """Add an ID column to TeamsDB.csv"""

    if not Path(teamsdb_path).exists():
        print(f"Error: {teamsdb_path} not found!")
        return

    if output_path is None:
        output_path = teamsdb_path.replace('.csv', '_with_ids.csv')

    with open(teamsdb_path, 'r', encoding='utf-8') as infile:
        reader = csv.reader(infile, delimiter=';')
        rows = list(reader)

    if not rows:
        print("Error: CSV file is empty!")
        return

    # Add 'ID' as the first column in header
    header = rows[0]
    header.insert(0, 'ID')

    # Add sequential IDs to each row
    team_id = 1
    for i in range(1, len(rows)):
        if rows[i] and rows[i][0].strip():  # Only if row has club name
            rows[i].insert(0, str(team_id))
            team_id += 1
        else:
            rows[i].insert(0, '')

    # Write output
    with open(output_path, 'w', encoding='utf-8', newline='') as outfile:
        writer = csv.writer(outfile, delimiter=';')
        writer.writerows(rows)

    print(f"Created {output_path} with ID column")
    print(f"Total teams: {team_id - 1}")
    print("\nReplace your original TeamsDB.csv with this file to use custom IDs")

if __name__ == '__main__':
    import sys

    # Default path
    default_path = 'NostalgiaManager/data/TeamsDB.csv'

    print("Team Screenshot Helper")
    print("=" * 70)
    print("Choose an option:")
    print("1. Show team ID mapping (to help name screenshot files)")
    print("2. Add ID column to TeamsDB.csv (for custom ID control)")
    print()

    choice = input("Enter choice (1 or 2): ").strip()

    csv_path = input(f"Path to TeamsDB.csv [{default_path}]: ").strip()
    if not csv_path:
        csv_path = default_path

    if choice == '1':
        generate_team_id_mapping(csv_path)
    elif choice == '2':
        output = input("Output path [TeamsDB_with_ids.csv]: ").strip()
        if not output:
            output = 'TeamsDB_with_ids.csv'
        add_id_column_to_csv(csv_path, output)
    else:
        print("Invalid choice!")

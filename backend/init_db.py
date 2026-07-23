import argparse
import clickhouse_connect

def initialize_database(set_emergency: bool = False):
    print("Connecting to ClickHouse...")
    client = clickhouse_connect.get_client(
        host="localhost",
        port=8123,
        username="default",
        password="resqpi123"
    )

    print("Creating database and table...")
    client.command("CREATE DATABASE IF NOT EXISTS resqpi_db")

    # Use the new resqpi_db database
    client.command("""
        CREATE TABLE IF NOT EXISTS resqpi_db.locations (
            location String,
            emergency Bool
        ) ENGINE = MergeTree()
        ORDER BY location
    """)

    # Check if the table is empty before inserting
    result = client.query("SELECT count() FROM resqpi_db.locations")
    row_count = result.result_rows[0][0]
    
    if row_count == 0:
        print(f"Inserting initial locations (emergency={set_emergency})...")
        data = [
            ['Location A', set_emergency],
            ['Location B', set_emergency],
            ['Location C', set_emergency],
            ['Location D', set_emergency],
            ['Location E', set_emergency],
        ]
        
        client.insert(
            'resqpi_db.locations', 
            data, 
            column_names=['location', 'emergency']
        )
        print("Locations successfully inserted into resqpi_db")
    else:
        print(f"Table already contains {row_count} rows.")
        if set_emergency:
            print("Updating all existing locations to emergency = True...")
            # ClickHouse mutations require a WHERE clause; WHERE 1=1 updates all rows
            client.command("ALTER TABLE resqpi_db.locations UPDATE emergency = true WHERE 1=1")
            print("✅ All existing locations updated to emergency = True!")
        else:
            print("Skipping insertion.")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Initialize ResQPi ClickHouse database")
    
    # Add flag
    parser.add_argument(
        "--emergency-true",
        "--emergency",
        action="store_true",
        help="Set all locations emergency status to True"
    )
    
    args = parser.parse_args()
    initialize_database(set_emergency=args.emergency_true)
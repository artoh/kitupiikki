#!/usr/bin/env python3
"""
Repair rows corrupted by the QByteArray-vs-text bind bug documented in
MIGRATION_NOTES.md (bug #1, fixed in mapToJson()/tositeroute.cpp on
2026-08-17). Any Postgres client database created with a pre-fix build may
have `json`-column rows where the stored value is the literal text of a
Postgres bytea hex-escape (e.g. "\\x7b226b6175...") instead of real JSON.

Because these columns are plain `text`, Postgres never rejected the bad
value — it's not real binary data, just a hex *string* that was written
where JSON should have been. That means it's fully recoverable: strip the
"\\x" prefix, hex-decode, decode as UTF-8, and the original JSON comes back
byte for byte.

Usage:
    python3 repair_bytea_json.py --host localhost --port 5432 \\
        --dbname im_kirjanpito --user kitsas --password '...' [--dry-run]
"""

import argparse

import psycopg2
import psycopg2.sql

# (table, id_column, json_column) for every table with a mapToJson()-written
# column. Keep this in sync with migrate_sqlite_to_pg.py's TABLE_ORDER.
TABLES = [
    ("Tili", "numero", "json"),
    ("Otsikko", "numero", "json"),
    ("Tilikausi", "alkaa", "json"),
    ("Kohdennus", "id", "json"),
    ("Kumppani", "id", "json"),
    ("Ryhma", "id", "json"),
    ("Tosite", "id", "json"),
    ("Vienti", "id", "json"),
    ("Tuote", "id", "json"),
    ("Rivi", "tosite", "json"),
    ("Vakioviite", "viite", "json"),
]


def decode_corrupted(value: str) -> str:
    """Reverse the bug: the stored text is the literal "\\x"-prefixed hex
    encoding of the original UTF-8 JSON bytes."""
    hex_part = value[2:]  # strip leading \x
    return bytes.fromhex(hex_part).decode("utf-8")


def repair(conn, dry_run: bool):
    cur = conn.cursor()
    total_fixed = 0
    for table, idcol, col in TABLES:
        cur.execute(f"SELECT {idcol}, {col} FROM {table} WHERE {col} LIKE '\\\\x%'")
        rows = cur.fetchall()
        for row_id, corrupted in rows:
            try:
                fixed = decode_corrupted(corrupted)
            except ValueError as e:
                print(f"  SKIP {table} {idcol}={row_id}: could not decode ({e})")
                continue
            print(f"  {table} {idcol}={row_id}:")
            print(f"    before: {corrupted[:80]}...")
            print(f"    after:  {fixed[:80]}")
            if not dry_run:
                # Table/column names come from the fixed TABLES list above, not
                # user input, and must stay unquoted so Postgres folds them to
                # the actual lowercase-stored names (as the app's own queries do).
                cur.execute(
                    f"UPDATE {table} SET {col} = %s WHERE {idcol} = %s",
                    (fixed, row_id),
                )
            total_fixed += 1
    if not dry_run:
        conn.commit()
    print(f"\n{'Would fix' if dry_run else 'Fixed'} {total_fixed} row(s).")


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--host", default="localhost")
    ap.add_argument("--port", type=int, default=5432)
    ap.add_argument("--dbname", required=True)
    ap.add_argument("--user", required=True)
    ap.add_argument("--password", required=True)
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    conn = psycopg2.connect(
        host=args.host, port=args.port, dbname=args.dbname,
        user=args.user, password=args.password,
    )
    try:
        repair(conn, args.dry_run)
    except Exception:
        conn.rollback()
        raise
    finally:
        conn.close()


if __name__ == "__main__":
    main()

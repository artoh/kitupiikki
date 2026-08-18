#!/usr/bin/env python3
"""
Migrate a Kitsas .kitsas SQLite file into a PostgreSQL database created by
the Postgres backend (kitsas/postgres/luo.sql schema).

Living/experimental script — expected to change as we find more SQLite vs.
Postgres discrepancies. See MIGRATION_NOTES.md in this directory for the
reasoning behind each conversion rule below; keep both files in sync.

Usage:
    python3 migrate_sqlite_to_pg.py path/to/kirjanpito.kitsas \\
        --host localhost --port 5432 --dbname im_kirjanpito \\
        --user kitsas --password '...'

Assumes the target Postgres database already exists and already has the
Kitsas schema applied (i.e. was created through the app's "New Postgres
client" flow, which runs postgres/luo.sql). That means seed rows
(Kohdennus id=0, Kumppani "Verohallinto") already exist in the target and
must NOT be duplicated from the source.
"""

import argparse
import json
import sqlite3
import sys

import psycopg2
import psycopg2.extras


# Tables in FK-safe insert order. Update this list if the schema grows.
TABLE_ORDER = [
    "Asetus",
    "Tili",
    "Otsikko",
    "Tilikausi",
    "Kohdennus",
    "Budjetti",
    "Kumppani",
    "KumppaniIban",
    "Ryhma",
    "KumppaniRyhmassa",
    "Tosite",
    "Tositeloki",
    "Vienti",
    "Merkkaus",
    "Liite",
    "Tuote",
    "Rivi",
    "Vakioviite",
]

# Tables with an identity/autoincrement `id` column whose Postgres sequence
# needs resyncing after importing rows with explicit ids.
IDENTITY_TABLES = [
    "Kohdennus",
    "Kumppani",
    "Ryhma",
    "Tosite",
    "Tositeloki",
    "Vienti",
    "Liite",
    "Tuote",
]

# Seed rows already present in a freshly-created Postgres client database
# (inserted by postgres/luo.sql) — skip these from the source to avoid
# primary-key conflicts.
SEED_SKIP = {
    "Kohdennus": [("id", 0)],
    "Kumppani": [("nimi", "Verohallinto")],
}


def sanitize_jsonb(value):
    """Tositeloki.data is strict jsonb in Postgres but loosely-typed in
    SQLite (see MIGRATION_NOTES.md). Map anything that isn't valid JSON,
    including '', to SQL NULL rather than letting Postgres reject the row."""
    if value is None:
        return None
    if isinstance(value, bytes):
        value = value.decode("utf-8", errors="replace")
    value = value.strip()
    if not value:
        return None
    try:
        json.loads(value)
    except (ValueError, TypeError):
        print(f"  WARNING: dropping malformed jsonb value: {value[:80]!r}", file=sys.stderr)
        return None
    return value


def as_text(value):
    """Any column that historically went through mapToJson() must be bound
    as plain text, never bytes — see MIGRATION_NOTES.md bug #1. sqlite3
    already returns str for TEXT-affinity columns, but be defensive in case
    a column comes back as bytes."""
    if isinstance(value, bytes):
        return value.decode("utf-8", errors="replace")
    return value


def fetch_rows(sconn, table):
    sconn.row_factory = sqlite3.Row
    cur = sconn.execute(f"SELECT * FROM {table}")
    return cur.fetchall()


def filter_seed_rows(table, rows):
    skip_conditions = SEED_SKIP.get(table)
    if not skip_conditions:
        return rows
    result = []
    for row in rows:
        skip = any(row[col] == val for col, val in skip_conditions)
        if skip:
            print(f"  skipping seed row already present in target: {table} {dict(row)}")
            continue
        result.append(row)
    return result


def migrate_table(sconn, pconn, table):
    rows = fetch_rows(sconn, table)
    rows = filter_seed_rows(table, rows)
    if not rows:
        print(f"{table}: nothing to import")
        return

    columns = rows[0].keys()
    pcur = pconn.cursor()

    values = []
    for row in rows:
        record = []
        for col in columns:
            v = row[col]
            if table == "Tositeloki" and col == "data":
                v = sanitize_jsonb(v)
            elif col == "json":
                v = as_text(v)
            record.append(v)
        values.append(record)

    col_list = ", ".join(columns)
    placeholders = ", ".join(["%s"] * len(columns))
    sql = f"INSERT INTO {table} ({col_list}) VALUES ({placeholders}) ON CONFLICT DO NOTHING"

    psycopg2.extras.execute_batch(pcur, sql, values)
    pconn.commit()
    print(f"{table}: imported {len(values)} row(s)")


def resync_sequences(pconn):
    pcur = pconn.cursor()
    for table in IDENTITY_TABLES:
        pcur.execute(
            "SELECT setval(pg_get_serial_sequence(%s, 'id'), "
            "COALESCE((SELECT MAX(id) FROM " + table + "), 1))",
            (table.lower(),),
        )
    pconn.commit()
    print("Resynced identity sequences for:", ", ".join(IDENTITY_TABLES))


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("sqlite_path", help="Path to the source .kitsas SQLite file")
    ap.add_argument("--host", default="localhost")
    ap.add_argument("--port", type=int, default=5432)
    ap.add_argument("--dbname", required=True, help="Target Postgres database (must already have the Kitsas schema)")
    ap.add_argument("--user", required=True)
    ap.add_argument("--password", required=True)
    ap.add_argument("--dry-run", action="store_true", help="Read and validate the source only, do not write to Postgres")
    args = ap.parse_args()

    sconn = sqlite3.connect(args.sqlite_path)

    if args.dry_run:
        for table in TABLE_ORDER:
            rows = fetch_rows(sconn, table)
            print(f"{table}: {len(rows)} row(s) in source")
        return

    pconn = psycopg2.connect(
        host=args.host, port=args.port, dbname=args.dbname,
        user=args.user, password=args.password,
    )

    try:
        for table in TABLE_ORDER:
            migrate_table(sconn, pconn, table)
        resync_sequences(pconn)
    except Exception:
        pconn.rollback()
        raise
    finally:
        pconn.close()
        sconn.close()

    print("Migration complete.")


if __name__ == "__main__":
    main()

#!/bin/sh

DBNAME=otus_messendger

psql -U postgres < create_db.sql
psql -U postgres -d ${DBNAME} < create_tables.sql


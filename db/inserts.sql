CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

INSERT INTO users (ID, name) VALUES (uuid_generate_v4(), 'user1');
INSERT INTO users (ID, name) VALUES (uuid_generate_v4(), 'user2');
INSERT INTO users (ID, name) VALUES (uuid_generate_v4(), 'user3');
INSERT INTO users (ID, name) VALUES (uuid_generate_v4(), 'user4');


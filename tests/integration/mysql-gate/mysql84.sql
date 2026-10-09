-- MySQL 8.4 accounts for the MySQL gate. caching_sha2_password is the
-- default; each account is used by one check so the server's cache state is
-- known (the first login of an account needs full authentication).
CREATE DATABASE gate;
CREATE USER 'sha2_tls'@'%' IDENTIFIED WITH caching_sha2_password BY 'gate-sha2-tls';
CREATE USER 'sha2_rsa'@'%' IDENTIFIED WITH caching_sha2_password BY 'gate-sha2-rsa';
CREATE USER 'sha2_timeout'@'%' IDENTIFIED WITH caching_sha2_password BY 'gate-timeout';
GRANT ALL ON gate.* TO 'sha2_tls'@'%', 'sha2_rsa'@'%', 'sha2_timeout'@'%';

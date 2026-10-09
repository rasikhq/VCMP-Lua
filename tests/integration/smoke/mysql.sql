-- Smoke test account (MySQL 8.4 and MariaDB 11).
CREATE DATABASE smoke;
CREATE USER 'smoke'@'%' IDENTIFIED BY 'smoke-password';
GRANT ALL ON smoke.* TO 'smoke'@'%';

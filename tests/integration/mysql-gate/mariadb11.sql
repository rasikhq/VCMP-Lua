-- MariaDB 11 accounts for the MySQL gate.
INSTALL SONAME 'auth_ed25519';
CREATE DATABASE gate;
CREATE USER 'native'@'%' IDENTIFIED VIA mysql_native_password USING PASSWORD('gate-native');
CREATE USER 'ed25519'@'%' IDENTIFIED VIA ed25519 USING PASSWORD('gate-ed25519');
GRANT ALL ON gate.* TO 'native'@'%', 'ed25519'@'%';

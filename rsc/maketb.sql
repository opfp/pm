DROP TABLE _index ;
DROP TABLE _main_cmk;
DROP TABLE _main_ukey;

CREATE TABLE _index ( /* no confusion with vaults because _ not allowed as vaultname[0] */
    ID CHAR(15) PRIMARY KEY,
    SALT CHAR(9),
    MASTER_KEY BINARY(23),
    UKEY TINYINT,
    VIS TINYINT
);

INSERT INTO _index (ID, UKEY, VIS)
    VALUES ("_main_cmk", 2, 1),
    ("_main_ukey", 1, 1),
    ("test", 1, 1);

CREATE TABLE _main_cmk (
    ID CHAR(32) PRIMARY KEY,
    SALT CHAR(9) NOT NULL,
    CIPHER BINARY(96) NOT NULL,
    VIS TINYINT,
    VALIDATE TINYTINT
);

CREATE TABLE _main_ukey (
    ID CHAR(32) PRIMARY KEY,
    SALT CHAR(9) NOT NULL,
    MASTER_KEY BINARY(23),
    CIPHER BINARY(96) NOT NULL,
    VIS TINYINT,
    VALIDATE TINYTINT
);

/*
DROP TABLE test;

CREATE TABLE test (
    ID CHAR(32) PRIMARY KEY,
    SALT CHAR(9) NOT NULL,
    MASTER_KEY BINARY(23),
    CIPHER BINARY(96) NOT NULL,
    VIS TINYINT,
    VALIDATE TINYTINT
);
*/

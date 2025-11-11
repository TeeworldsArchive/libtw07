#include "../lib/datefile.h"

int main(int argc, const char **argv)
{
    libtw07_datafileReader Reader;
    libtw07_datafile_reader_init(&Reader);
    libtw07_datafile_reader_open(&Reader, "testmap.map");

    uint32_t Crc = libtw07_datafile_crc(&Reader);
    SHA256_DIGEST Sha256 = libtw07_datafile_sha256(&Reader);

    char aSha256[SHA256_MAXSTRSIZE];
    sha256_str(Sha256, aSha256, sizeof(aSha256));

    libtw07_print("test", "crc is %u, sha256 is %s", Crc, aSha256);
    libtw07_datafile_reader_destroy(&Reader);
    return 0;
}

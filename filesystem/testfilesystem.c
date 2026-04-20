#include <stdio.h>
#include <string.h>
#include "file_system.h"

void test_create_and_ls(FileSystem *fs)
{
    printf("\n--- TEST: CREATE + LS ---\n");

    fs_create(fs, "file1", FILE_TYPE);
    fs_create(fs, "file2", FILE_TYPE);
    fs_create(fs, "dir1", DIR_TYPE);

    fs_ls(fs);
}

void test_write_and_read_basic(FileSystem *fs)
{
    printf("\n--- TEST: WRITE + READ BASIC ---\n");

    int inode = fs_lookup(fs, "file1");

    char data[] = "hello world";
    fs_write(fs, inode, data, strlen(data));

    char buffer[100];
    int read = fs_read(fs, inode, buffer, sizeof(buffer));

    buffer[read] = '\0';

    printf("READ OUTPUT: %s\n", buffer);
}

void test_overwrite(FileSystem *fs)
{
    printf("\n--- TEST: OVERWRITE FILE ---\n");

    int inode = fs_lookup(fs, "file1");

    fs_write(fs, inode, "first write", 12);
    fs_write(fs, inode, "second write longer", 20);

    char buffer[100];
    int read = fs_read(fs, inode, buffer, sizeof(buffer));

    buffer[read] = '\0';

    printf("EXPECTED: second write longer\n");
    printf("ACTUAL:   %s\n", buffer);
}

void test_multiple_blocks(FileSystem *fs)
{
    printf("\n--- TEST: MULTI BLOCK WRITE ---\n");

    int inode = fs_lookup(fs, "file2");

    char big[2000];
    for (int i = 0; i < 1999; i++)
        big[i] = 'A';
    big[1999] = '\0';

    fs_write(fs, inode, big, strlen(big));

    char buffer[2000];
    int read = fs_read(fs, inode, buffer, sizeof(buffer));

    printf("Bytes read: %d\n", read);
    printf("First char: %c\n", buffer[0]);
    printf("Last char: %c\n", buffer[read - 1]);
}

void test_delete(FileSystem *fs)
{
    printf("\n--- TEST: DELETE ---\n");

    fs_create(fs, "temp", FILE_TYPE);

    int inode = fs_lookup(fs, "temp");

    fs_write(fs, inode, "to be deleted", 13);

    fs_delete(fs, "temp");

    int lookup = fs_lookup(fs, "temp");

    printf("Lookup after delete (should be -1): %d\n", lookup);
}

void test_directory_protection(FileSystem *fs)
{
    printf("\n--- TEST: DIRECTORY SAFETY ---\n");

    int dir = fs_lookup(fs, "dir1");

    int res = fs_write(fs, dir, "should fail", 11);

    printf("Write to directory result (should be -1): %d\n", res);
}

void test_reuse_blocks(FileSystem *fs)
{
    printf("\n--- TEST: BLOCK REUSE (BITMAP) ---\n");

    fs_create(fs, "a", FILE_TYPE);
    fs_create(fs, "b", FILE_TYPE);

    int a = fs_lookup(fs, "a");
    int b = fs_lookup(fs, "b");

    fs_write(fs, a, "AAAAA", 5);
    fs_write(fs, b, "BBBBB", 5);

    char buf[10];

    fs_read(fs, a, buf, 5);
    buf[5] = '\0';
    printf("A: %s\n", buf);

    fs_read(fs, b, buf, 5);
    buf[5] = '\0';
    printf("B: %s\n", buf);
}

void test_copy_on_write(FileSystem *fs)
{
    printf("\n--- TEST: COPY ON WRITE UPDATE ---\n");

    fs_create(fs, "cow.txt", FILE_TYPE);

    int inode = fs_lookup(fs, "cow.txt");

    if (inode == -1)
    {
        printf("FAIL: file not created\n");
        return;
    }

    fs_write(fs, inode, "OLD DATA", 8);

    char buffer[200];

    int read_size = fs_read(fs, inode, buffer, 200);
    if (read_size < 0)
    {
        printf("FAIL: read error after first write\n");
        return;
    }

    buffer[read_size] = '\0';
    printf("Initial file contents: %s\n", buffer);

    int old_size = fs_get_size(fs, inode);

    fs_write(fs, inode, "NEW DATA HERE", 13);

    read_size = fs_read(fs, inode, buffer, 200);
    if (read_size < 0)
    {
        printf("FAIL: read error after overwrite\n");
        return;
    }

    buffer[read_size] = '\0';
    printf("Updated file contents: %s\n", buffer);

    int new_size = fs_get_size(fs, inode);

    if (old_size != new_size)
        printf("PASS: file size updated correctly\n");

    if (strstr(buffer, "NEW DATA HERE") != NULL)
        printf("PASS: overwrite successful\n");
    else
        printf("FAIL: overwrite did not apply\n");
}

int main()
{
    printf("=== FILE SYSTEM TESTS ===\n");

    FileSystem fs;
    fs_init(&fs);

    test_create_and_ls(&fs);
    test_write_and_read_basic(&fs);
    test_overwrite(&fs);
    test_multiple_blocks(&fs);
    test_delete(&fs);
    test_directory_protection(&fs);
    test_reuse_blocks(&fs);
    test_copy_on_write(&fs);

    printf("\n=== TESTS COMPLETE ===\n");

    return 0;
}
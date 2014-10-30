extern void display_char(int, int, char);

/*
 * C entry
 */
void cstart()
{
    for (unsigned char c = 0;; ++c)
        display_char(20, 10, c);
}

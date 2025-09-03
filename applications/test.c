int main()
{
    int a = 42;
    int b = 15;

    int result = ((a & b) | (a ^ b)) << 1;

    return result;
}
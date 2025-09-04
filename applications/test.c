union data
{
    int i;
    char c;
};

int main()
{
    union data ic;
    ic.i = 65;
    ic.c = 'm';
    return ic.i;
}
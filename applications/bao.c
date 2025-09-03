int main()
{
    int x = 0;
    int total = 0;

    do
    {
        if (x == 2)
        {
            x++;
            continue;
        }
        else if (x == 5)
            total += 50;
        else if (x > 6)
            break;
        else
            total += x;

        x++;
    } while (x < 10);

    return total;
}

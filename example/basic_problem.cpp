void again(int value);
void dec(int value, int by);

void again(int value)
{
    if(value < 0)
        return;
    dec(value, 1);
}

void dec(int value, int by)
{
    again(value - by);
}

int main()
{
    again(2000000); // stack explosion
}

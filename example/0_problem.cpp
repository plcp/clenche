void again(int value);
void dec(int value, int by);

void again(int value)
{
    // ** do some processing **
    if(value < 0)
        return; // ** sometimes stop processing **

    dec(value, 1); // -> again calls dec, and dec never returns.
}

void dec(int value, int by)
{
    again(value - by); // -> dec calls again, and again never returns.
}

int main()
{
    again(2000000); // Here, our stack explodes, sadly.
}

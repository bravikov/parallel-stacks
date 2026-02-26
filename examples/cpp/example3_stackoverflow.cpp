void recursive(int & count)
{
    int current_count = count++;
    recursive(count);
}

int main()
{
    int count = 0;
    recursive(count);
    return 0;
}

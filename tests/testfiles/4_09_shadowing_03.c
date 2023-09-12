int main() {
    int a = 5;
    {
        int b = 4;
        a = 6;
        int a = 12;
    }
    {
        int c = 7;
        int d = 5;
        int e = 4;
        a = a * c;
        {
            a = a - d;
        }
        a = a / e;
        {
            a = a + 6;
        }
        int a = 9;
    }
    return a;
}
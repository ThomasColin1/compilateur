int main(){
    int a = 2;
    int b = 3;
    {
        a = a + b;
        {
            a = a + b;
            int a = 5;
            int b = 9;
            a = a + b;
        }
        int a = 12;
        int b = 7;
        a = a + b;
    }
    return a; 
}
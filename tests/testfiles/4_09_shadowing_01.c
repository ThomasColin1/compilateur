int main(){
    int a = 2;
    {
        a = 12;
        int a = 5;
    }
    return a; 
}
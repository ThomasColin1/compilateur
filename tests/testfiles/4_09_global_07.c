int somme(int a, int b, int c, int d, int e, int f){
    a = produit(a, b);
    b = division(f, c);
    return a+b+c+d+e+f;
}

int main(){
    int s = 0;
    s = somme(1, 2, 3, 4, 5);
    return s;    
}
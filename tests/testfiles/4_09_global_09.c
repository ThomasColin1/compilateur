int somme(int a, int b, int c, int d, int e, int f){
    a = produit(a, b);
    b = division(f, c);
    return a+b+c+d+e+f;
}

int main() {
    int a = somme(1, 2, 3, 4, 5, 6, 7);
    return a;
}
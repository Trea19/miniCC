int get_sum(int a, int b){
    return a + b;
}

int main(){
    int a, b, c;
    a = read();
    b = read();
    
    c = get_sum(a, b);
    write(c);
    
    return 0;
}

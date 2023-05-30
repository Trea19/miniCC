int sub_one(int x){
   return x - 1;
}

int main()
{
    int n = 3;
	
    while (n) {
    	sub_one(n);
    }
    write(n);
    return 0;
}

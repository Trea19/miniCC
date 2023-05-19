int main() {
	int n = 1;
	//n = 1;

	//print("The first 10 number of the fibonacci sequence:");
	while (n <= 10) {
		if (n = 5) {
		    break;
		}
		n = n + 1;
	}

	fib(n)
	return 0;
}

int fib(int n) {
	if (n <= 2) {
		return 1;
	}
	return fib(n - 1) + fib(n - 2);
}


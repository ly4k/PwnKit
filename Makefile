.PHONY: all clean
all: PwnKit PwnKit32

PwnKit:
	gcc -shared PwnKit.c -o PwnKit -Wl,-e,entry -fPIC

PwnKit32:
	gcc -shared -m32 PwnKit.c -o PwnKit32 -Wl,-e,entry -fPIC

clean:
	rm PwnKit PwnKit32

KERN_DIR = /home/cyj/workspacecdt/jz2440-android-kernel-2.6.27

all:
	make -C $(KERN_DIR) M=`pwd` modules 

clean:
	make -C $(KERN_DIR) M=`pwd` modules clean
	rm -rf modules.order

obj-m	+= key.o


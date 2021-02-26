#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/proc_fs.h>

/*
#define MY_MAJOR            52
#define MY_MAX_MINORS       5
*/
#define MAX_ARR_LEN         512 // must be greater than MAX_NUM_DIGITS !!!
#define MAX_NUM_DIGITS      20 // this is how many digits there are in 2^64 - 1 


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tcherezovs");
MODULE_DESCRIPTION("Lab1");

struct my_device_data {
    struct cdev cdev;
    
    // the records (lengths as strings) are space seprated
    char arr[MAX_ARR_LEN];
    size_t size;
    size_t start;
};

static dev_t dev;
static struct cdev c_dev;
static struct class* cl;
static struct proc_dir_entry* entry;

void  my_device_data_init(struct my_device_data* data){
    data->size = 0;
    data->start = 0;
}

size_t get_num_digits(size_t d){
    size_t num_digits = d == 0 ? 1 : 0;
    while (d) {
        d /= 10;
        num_digits++;
    }
    return num_digits;
}

void remove_first_record(struct my_device_data* data){
    while (data->size && data->arr[data->start] != ' '){
        data->start = (data->start + 1) % MAX_ARR_LEN;
        data->size--;
    }
    data->start = (data->start + 1) % MAX_ARR_LEN;
    data->size--;
}

void push_back(struct my_device_data* data, size_t d){
    // make space for the data if there's not enough
    size_t len = min(get_num_digits(d), MAX_NUM_DIGITS);
    while (data->size + len >= MAX_ARR_LEN){
        remove_first_record(data);
    }

    // insert the d into data->arr as chars
    size_t i;
    for (i = 0; i < len; ++i){
        size_t pos = (data->start + data->size) % MAX_ARR_LEN;
        data->arr[pos] = d % 10 + '0';
        d /= 10;
        data->size++;
    }
    data->arr[(data->start + data->size) % MAX_ARR_LEN] = ' ';
    
    data->size++;

}



//struct my_device_data devs[MY_MAX_MINORS];

static int my_open(struct inode* inode, struct file* file)
{
    struct my_device_data* data;
    data = container_of(inode->i_cdev, struct my_device_data, cdev);

    file->private_data = data;

    my_device_data_init(data);
    return 0;
}

static ssize_t my_read(struct file* file, char __user *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data* data;

    data = (struct my_device_data*) file->private_data;
    ssize_t len = min(data->size - *offset, size);

    if (len <= 0)
        return 0;

    if (copy_to_user(user_buffer, data->arr + *offset, len)){
        return -EFAULT;
    }    
    
    *offset += len;

    return len;
}

static ssize_t my_write(struct file* file, const char __user *user_buffer, size_t size, loff_t* offset)
{
    struct my_device_data* data;
    data = (struct my_device_data*) file->private_data;
    push_back(data, size);
    printk(KERN_INFO "writing to var1");

    return size;
}

static int my_close(struct inode* i, struct file* f){
    printk(KERN_INFO "closing var1");
    return 0;
}

const struct file_operations my_cdev_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .release = my_close,
    .read = my_read,
    .write = my_write
};

static ssize_t proc_write(struct file* file, const char __user *user_buffer, size_t size, loff_t* offset){
    printk(KERN_DEBUG "Attempt to write to proc file");
    return -1;
}

const struct file_operations my_proc_fops = {
    .owner = THIS_MODULE,
    .read = my_read,
    .write = proc_write
};

int __init my_init_module(void)
{
    /*int i, err;

    err = register_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS, "my_device_driver");

    if (err != 0) {
        // report error
        return err;
    }

    for (i = 0; i < MY_MAX_MINORS; ++i) {
        cdev_init(&devs[i].cdev, &my_fops);
        cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1);
    }   
    */

    // create and register proc device
    entry = proc_create("var1", 0444, NULL, &my_proc_fops);

    // create and register character device
    if (alloc_chrdev_region(&dev, 0, 1, "dev_var1") < 0){
        return -1;
    }
    if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL){
        unregister_chrdev_region(dev, 1);
        return -1;
    }
    if (device_create(cl, NULL, dev, NULL, "var1") == NULL){
        class_destroy(cl);
        unregister_chrdev_region(dev, 1);
        return -1;
    }
    cdev_init(&c_dev, &my_cdev_fops);
    if (cdev_add(&c_dev, dev, 1) == -1){
        device_destroy(cl, dev);
        class_destroy(cl);
        unregister_chrdev_region(dev, 1);
        return -1;
    }
    printk(KERN_INFO "lab1 module initialized\n");
    return 0;
}

void __exit my_cleanup_module(void)
{
    /*
    int i;
    for (i = 0; i < MY_MAX_MINORS; ++i) {
        // release devs[i] fields
        cdev_del(&devs[i].cdev);
    }
    unregister_chrdev_region(MKDEV(MY_MAJOR, 0, MY_MAX_MINORS);
    */

    // cleanup proc
    proc_remove(entry);

    // cleanup character device
    cdev_del(&c_dev);
    device_destroy(cl, dev);
    class_destroy(cl);
    unregister_chrdev_region(dev, 1);
    printk(KERN_INFO "lab1 module cleaned\n");
}

module_init(my_init_module);
module_exit(my_cleanup_module);




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

#define MAX_ARR_LEN         512 // must be greater than MAX_NUM_DIGITS !!!
#define MAX_NUM_DIGITS      20 // this is how many digits there are in 2^64 - 1 

#define EXTRA_TASK


MODULE_LICENSE("GPL");
MODULE_AUTHOR("Tcherezovs");
MODULE_DESCRIPTION("Lab1");

struct my_device_data {
    // the records (lengths as strings) are space seprated
#ifdef EXTRA_TASK
    size_t total;
#endif // EXTRA_TASK
    char arr[MAX_ARR_LEN];
    size_t size;
    size_t start;
};

static dev_t dev;
static struct cdev c_dev;
static struct class* cl;
static struct proc_dir_entry* entry;

static struct my_device_data data;

void  my_device_data_init(struct my_device_data* data){
    data->size = 0;
    data->start = 0;
    #ifdef EXTRA_TASK
    data->total = 0;
    #endif // EXTRA_TASK
}

ssize_t mmin (ssize_t a, ssize_t b){
    return a < b ? a : b;
}

size_t get_num_digits(size_t d){
    size_t num_digits = d == 0 ? 1 : 0;
    while (d) {
        d /= 10;
        num_digits++;
    }
    return num_digits;
}

void to_chars(size_t x, char* buf, size_t num_digits){
    size_t i;
    for (i = 1 ; i <= num_digits; ++i){
        size_t pos = num_digits - i;
        buf[pos] = x % 10 + '0';
        x /= 10;
    }
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
    size_t len = mmin(get_num_digits(d), MAX_NUM_DIGITS);
    while (data->size + len >= MAX_ARR_LEN){
        remove_first_record(data);
    }

    // insert the d into data->arr as chars
    data->size += len;
    size_t i;
    for (i = 1 ; i <= len; ++i){
        size_t pos = (data->start + data->size - i) % MAX_ARR_LEN;
        data->arr[pos] = d % 10 + '0';
        d /= 10;
    }
    data->arr[(data->start + data->size) % MAX_ARR_LEN] = ' ';
    
    data->size++;

}

static int my_open(struct inode* inode, struct file* file)
{
    
    return 0;
}

static ssize_t my_read(struct file* file, char __user *user_buffer, size_t size, loff_t *offset)
{
    ssize_t len;

#ifdef EXTRA_TASK
    char buf[MAX_NUM_DIGITS + 2];
    size_t num_digits = mmin(get_num_digits(data.total), MAX_NUM_DIGITS);
    to_chars(data.total, buf, num_digits);
    buf[num_digits] = '\n';
    buf[num_digits+1] = '\r';
    len = mmin(num_digits + 2 - *offset, size);
    if (len <= 0)
        return 0;
    if (copy_to_user(user_buffer, buf + *offset, len)){
        return -EFAULT;
    }
    *offset += len;
#else 
    len = mmin(data.size - *offset, size - 2);

    if (len <= 0)
        return 0;

    if (copy_to_user(user_buffer, data.arr + *offset, len)){
        return -EFAULT;
    }

    *offset += len;

    // if we have read all the data in data.arr, output new line
    if (*offset == data.size){
        copy_to_user(user_buffer + len, "\n\r", 2);
        len += 2;
    }
#endif // EXTRA_TASK
    

    return len;
}

static ssize_t my_write(struct file* file, const char __user *user_buffer, size_t size, loff_t* offset)
{

#ifndef EXTRA_TASK
    push_back(&data, size - 1);
#else 
    data.total += size - 1;
#endif // EXTRA_TASK
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

    my_device_data_init(&data);
    
    printk(KERN_INFO "lab1 module initialized\n");
    return 0;
}

void __exit my_cleanup_module(void)
{
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




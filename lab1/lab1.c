#include <linux/fs.h>
#include <linux/cdev.h>


#define MY_MAJOR            52
#define MY_MAX_MINORS       5
#define MAX_LEN_NUM_CHARS   40
typedef size_t data_t;


struct my_device_data {
    struct cdev cdev;

    data_t num_chars_arr[MAX_LEN_NUM_CHARS];
    size_t size;
    size_t start;
};

void  my_device_data_init(struct my_device_data* data){
    data->size = 0;
    data->start = 0;
}

void push_back(struct my_device_data* data, data_t d){
    data->num_chars_arr[(start+size)%MAX_LEN_NUM_CHARS] = d;
    if (data->size < MAX_LEN_NUM_CHARS){
        data->size++;
    }  
}

struct my_device_data devs[MY_MAX_MINORS];

static int my_open(struct inode* inode, struct file* file)
{
    struct my_device_data* data;
    data = container_of(inode->i_cdev, struct my_device_data, cdev);

    file->private_data = data;

    my_device_data_init(data);
}

static int my_read(struct file* file, char __user *user_buffer, size_t size, loff_t *offset)
{
    struct my_device_data* data;

    data = (struct my_device_data*) file->private_data;
}

static int my_write(struct file* file, const char __user *user_buffer, size_t size, loff_t* offset)
{
    struct my_device_data* data;
    data = (struct my_device_data*) file->private_data;
    push_back(data, size);

    return size;
}

const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release,
    .unlocked_ioctl = my_ioctl
};

int init_module(void)
{
    int i, err;

    err = register_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS, "my_device_driver");

    if (err != 0) {
        // report error
        return err;
    }

    for (i = 0; i < MY_MAX_MINORS; ++i) {
        cdev_init(&devs[i].cdev, &my_fops);
        cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, I), 1);
    }

}

void cleanup_module(void)
{
    int i;
    for (i = 0; i < MY_MAX_MINORS; ++i) {
        // release devs[i] fields
        cdev_del(&devs[i].cdev);
    }
    unregister_chrdev_region(MKDEV(MY_MAJOR, 0, MY_MAX_MINORS);
}





#ifndef _BLOCK_INTERRUPT_H_
#define _BLOCK_INTERRUPT_H_

class blocking_interrupt
{
    public:
        blocking_interrupt(const std::string &dev_path);
        virtual ~blocking_interrupt();

        void initialize();
        int wait_for_interrupt();

    protected:
        std::string devpath;
        int fd;

    private:
        // block to use the default constructor
        blocking_interrupt();
};

#endif
/*  vim:  set ts=4 sts=4 sw=4 et cin   : */

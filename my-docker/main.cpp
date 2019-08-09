#include <stdio.h>
#include <sched.h>   // for clone
#include <fcntl.h>
#include <new>
#include <cstdlib>
#include <unistd.h>
#include <string>
#include <curl/curl.h>
#include <sys/stat.h> // to check folder existence
#include <sys/wait.h>
#include <sys/mount.h>

const std::string root = "root";
const std::string cgroup_root = "/sys/fs/cgroup/pids";
const std::string cgroup_name = "/zhuzilin_container";
const std::string cgroup_folder = cgroup_root + cgroup_name;

void setup_variables() {
    clearenv();   // remove all environment variables for this process.
    // set basic environment variables
    setenv("TERM", "xterm-256color", 0);
    setenv("PATH", "/bin/:/sbin/:usr/bin:/usr/sbin", 0);
}

void setup_root(const char* folder) {
    chroot(folder);
    chdir("/");
}

void write_rule(const std::string& path, const std::string& value) {
    int fp = open(path.c_str(), O_WRONLY | O_APPEND );
    write(fp, value.c_str(), value.size());
    close(fp);
}

void limit_process_creation() {
    // create a folder under the cgroup position
    // https://unix.stackexchange.com/questions/270288/mkdir-under-cgroup-creates-files-along-with-directories
    // the cgroup file system will create the file needed
    const std::string pid  = std::to_string(getpid());

    write_rule(cgroup_folder + "/cgroup.procs", pid);
    write_rule(cgroup_folder + "/notify_on_release", "1");
    write_rule(cgroup_folder + "/pids.max", "5");
}

char *stack_memory() {
    const int stackSize = 65536;
    // nothrow: return nullptr if failed
    auto * stack = new (std::nothrow) char[stackSize];
    if(stack == nullptr) {
        printf("fail to allocate memory");
        exit(EXIT_FAILURE);
    }
    return stack + stackSize;
}

int run(void *args) {
    char *_args[] = {(char *)"/bin/sh", nullptr};
    execvp(_args[0], _args);
}

int jail(void *args) {
    printf("add cgroup limits\n");
    limit_process_creation();
    printf("set enrionment variables\n");
    setup_variables();
    printf("set %s as root dir\n", root.c_str());
    setup_root(root.c_str());
    printf("mount proc\n");
    mount("proc", "/proc", "proc", 0, 0);
    
    char* stack = stack_memory();
    printf("start container!!!\n");
    clone(run, stack, SIGCHLD, 0);
    //delete[] stack;

    wait(nullptr);
    printf("end container.\n");
    printf("unmount proc\n");
    umount("/proc");
    return EXIT_SUCCESS;
}

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

// use libcurl to down load alphine
// from https://curl.haxx.se/libcurl/c/url2file.html
void download(std::string dir) {
    std::string filename = dir + "/alphine.tar.gz";
    // initial curl handler
    curl_global_init(CURL_GLOBAL_ALL);

    CURL *curl = curl_easy_init();
    if (curl) {
        CURLcode res;
        /* set URL to get here */ 
        curl_easy_setopt(curl, CURLOPT_URL, 
            "http://nl.alpinelinux.org/alpine/v3.7/releases/x86_64/alpine-minirootfs-3.7.0-x86_64.tar.gz");
        /* send all data to this function  */ 
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        /* open the file */ 
        FILE* pagefile = fopen(filename.c_str(), "wb");
        if(pagefile) {
            /* write the page body to this file handle */ 
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, pagefile);
    
            /* get it! */ 
            curl_easy_perform(curl);
    
            /* close the header file */ 
            fclose(pagefile);
        }
        curl_easy_cleanup(curl);

        curl_global_cleanup();
    }
}

int main(int argc, char* argv[]) {
    /*** preparation ***/
    // check if root is an empty folder
    struct stat info;
    if (stat(root.c_str(), &info) != 0) {
        printf("cannot access %s. probably not exist\n", root.c_str());
        printf("try to mkdir...\n");
        if(mkdir(root.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            printf("failed to mkdir, exit now.\n");
            return 0;
        }
        printf("successfully create folder!");
    }
    else if (S_ISDIR(info.st_mode)) {
        printf("%s does exist as a directory\n", root.c_str());
        printf("cleaning folder..\n");
        system(("rm -r " + root + "/*").c_str());
        printf("folder cleaned.\n");
    }
    else {
        printf("%s is not a directory, probably a file?\n", root.c_str());
        return 0;
    }
    
    // tmp folder for store images
    if (stat("tmp", &info) != 0) {
        printf("cannot access %s. probably not exist\n", "tmp");
        printf("try to mkdir...\n");
        if(mkdir("tmp", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)) {
            printf("failed to mkdir, exit now.\n");
            return 0;
        }
        printf("successfully create folder!");
    }
    else if (!S_ISDIR(info.st_mode)) {
        printf("tmp is not a directory, probably a file?\n");
        return 0;
    }

    printf("start downloading alphine...\n");
    download("tmp");
    printf("finish downloading alphine.\n");

    system(("tar -zxf tmp/alphine.tar.gz -C" + root).c_str());

    // create cgroup
    printf("create cgroup: pid:%s\n", cgroup_name.c_str());
    mkdir(cgroup_folder.c_str(), S_IRUSR | S_IWUSR);

    // clone create a new process
    // but different from fork, this process can share
    // parts with parent, like address space, table descriptor, table of signal handlers etc.
    // arguments:
    // fn: the functon will be executed
    // child_stack: location of stack used by child the child process
    //    notice that we need to pass the top most part of the stack
    //    notice that for raw system call(not the C version), 
    //        if pass a null pointer, child will copy parent stack.
    // flags:
    //   SIGCHLD:      signal parent of child's termination
    //   CLONE_NEWUTS: get a copy of the global UTS
    //   CLONE_NEWPID: create the process in a new PID namespace.
    // notice: clone itself will free the stack
    char* stack = stack_memory();
    clone(jail, stack, SIGCHLD | CLONE_NEWUTS | CLONE_NEWPID, 0);
    wait(nullptr);
    // delete cgroup
    printf("delete cgroup: pid:%s\n", cgroup_name.c_str());
    system(("cgdelete pids:" + cgroup_name).c_str());
    return EXIT_SUCCESS;
}

/*
#include <ctime>
#include <functional>
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <sys/wait.h>

class Command
{
public:
    int             ExitStatus = 0;
    std::string     Command;
    std::string     StdIn;
    std::string     StdOut;
    std::string     StdErr;

    void execute()
    {
        const int READ_END = 0;
        const int WRITE_END = 1;

        int infd[2] = {0, 0};
        int outfd[2] = {0, 0};
        int errfd[2] = {0, 0};

        auto cleanup = [&]() {
            ::close(infd[READ_END]);
            ::close(infd[WRITE_END]);

            ::close(outfd[READ_END]);
            ::close(outfd[WRITE_END]);

            ::close(errfd[READ_END]);
            ::close(errfd[WRITE_END]);
        };

        auto rc = ::pipe(infd);
        if(rc < 0)
        {
            throw std::runtime_error(std::strerror(errno));
        }

        rc = ::pipe(outfd);
        if(rc < 0)
        {
            ::close(infd[READ_END]);
            ::close(infd[WRITE_END]);
            throw std::runtime_error(std::strerror(errno));
        }

        rc = ::pipe(errfd);
        if(rc < 0)
        {
            ::close(infd[READ_END]);
            ::close(infd[WRITE_END]);

            ::close(outfd[READ_END]);
            ::close(outfd[WRITE_END]);
            throw std::runtime_error(std::strerror(errno));
        }

        auto pid = fork();
        if(pid > 0) // PARENT
        {
            ::close(infd[READ_END]);    // Parent does not read from stdin
            ::close(outfd[WRITE_END]);  // Parent does not write to stdout
            ::close(errfd[WRITE_END]);  // Parent does not write to stderr

            if(::write(infd[WRITE_END], StdIn.data(), StdIn.size()) < 0)
            {
                throw std::runtime_error(std::strerror(errno));
            }
            ::close(infd[WRITE_END]); // Done writing
        }
        else if(pid == 0) // CHILD
        {
            ::dup2(infd[READ_END], STDIN_FILENO);
            ::dup2(outfd[WRITE_END], STDOUT_FILENO);
            ::dup2(errfd[WRITE_END], STDERR_FILENO);

            ::close(infd[WRITE_END]);   // Child does not write to stdin
            ::close(outfd[READ_END]);   // Child does not read from stdout
            ::close(errfd[READ_END]);   // Child does not read from stderr

            ::execl("/bin/bash", "bash", "-c", Command.c_str(), nullptr);
            ::exit(EXIT_SUCCESS);
        }

        // PARENT
        if(pid < 0)
        {
            cleanup();
            throw std::runtime_error("Failed to fork");
        }

        int status = 0;
        ::waitpid(pid, &status, 0);

        std::array<char, 256> buffer;

        ssize_t bytes = 0;
        do
        {
            bytes = ::read(outfd[READ_END], buffer.data(), buffer.size());
            StdOut.append(buffer.data(), bytes);
        }
        while(bytes > 0);

        do
        {
            bytes = ::read(errfd[READ_END], buffer.data(), buffer.size());
            StdErr.append(buffer.data(), bytes);
        }
        while(bytes > 0);

        if(WIFEXITED(status))
        {
            ExitStatus = WEXITSTATUS(status);
        }

        cleanup();
    }
};
std::string execCommand(const std::string cmd, int& out_exitStatus)
{
    out_exitStatus = 0;
    auto pPipe = ::popen(cmd.c_str(), "r");
    if(pPipe == nullptr)
    {
        throw std::runtime_error("Cannot open pipe");
    }

    std::array<char, 256> buffer;

    std::string result;

    while(not std::feof(pPipe))
    {
        auto bytes = std::fread(buffer.data(), 1, buffer.size(), pPipe);
        result.append(buffer.data(), bytes);
    }

    auto rc = ::pclose(pPipe);

    if(WIFEXITED(rc))
    {
        out_exitStatus = WEXITSTATUS(rc);
    }

    return result;
}
void callback(
        const boost::system::error_code& error,
        std::size_t,
        char recv_str[]) {
    if (error)
    {
        std::cout << error.message() << '\n';
    }
    else
    {
        std::cout << recv_str << '\n';
    }
}

int main()
{
    Command cmd;
    cmd.Command = "ls";
   // cmd.StdIn = R"(Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua.)";

    cmd.execute();

    std::cout << cmd.StdOut;
    std::cerr << cmd.StdErr;
    std::cout << "Exit Status: " << cmd.ExitStatus << "\n";
    try
    {
        boost::asio::io_service io_service;
        boost::asio::ip::tcp::endpoint ep(boost::asio::ip::tcp::v4(), 3303);
        boost::asio::ip::tcp::acceptor acceptor(io_service, ep);

        for (;;)
        {
            boost::asio::ip::tcp::socket socket(io_service);
            acceptor.accept(socket);

            char recv_str[1024] = {};
            socket.async_receive(
                    boost::asio::buffer(recv_str),
                    std::bind(callback, std::placeholders::_1, std::placeholders::_2, recv_str));
            //socket.get_executor();

            io_service.run();
            //socket.get_executor();
            io_service.restart();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}*/
#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <sys/wait.h>

//using boost::asio::ip::tcp;

class Command
{
public:
    int             ExitStatus = 0;
    std::string     Command;
    std::string     StdIn;
    std::string     StdOut;
    std::string     StdErr;

    void execute()
    {
        const int READ_END = 0;
        const int WRITE_END = 1;

        int infd[2] = {0, 0};
        int outfd[2] = {0, 0};
        int errfd[2] = {0, 0};

        auto cleanup = [&]() {
            ::close(infd[READ_END]);
            ::close(infd[WRITE_END]);

            ::close(outfd[READ_END]);
            ::close(outfd[WRITE_END]);

            ::close(errfd[READ_END]);
            ::close(errfd[WRITE_END]);
        };

        auto rc = ::pipe(infd);
        if(rc < 0)
        {
            throw std::runtime_error(std::strerror(errno));
        }

        rc = ::pipe(outfd);
        if(rc < 0)
        {
            ::close(infd[READ_END]);
            ::close(infd[WRITE_END]);
            throw std::runtime_error(std::strerror(errno));
        }

        rc = ::pipe(errfd);
        if(rc < 0)
        {
            ::close(infd[READ_END]);
            ::close(infd[WRITE_END]);

            ::close(outfd[READ_END]);
            ::close(outfd[WRITE_END]);
            throw std::runtime_error(std::strerror(errno));
        }

        auto pid = fork();
        if(pid > 0) // PARENT
        {
            ::close(infd[READ_END]);    // Parent does not read from stdin
            ::close(outfd[WRITE_END]);  // Parent does not write to stdout
            ::close(errfd[WRITE_END]);  // Parent does not write to stderr

            if(::write(infd[WRITE_END], StdIn.data(), StdIn.size()) < 0)
            {
                throw std::runtime_error(std::strerror(errno));
            }
            ::close(infd[WRITE_END]); // Done writing
        }
        else if(pid == 0) // CHILD
        {
            ::dup2(infd[READ_END], STDIN_FILENO);
            ::dup2(outfd[WRITE_END], STDOUT_FILENO);
            ::dup2(errfd[WRITE_END], STDERR_FILENO);

            ::close(infd[WRITE_END]);   // Child does not write to stdin
            ::close(outfd[READ_END]);   // Child does not read from stdout
            ::close(errfd[READ_END]);   // Child does not read from stderr

            ::execl("/bin/bash", "bash", "-c", Command.c_str(), nullptr);
            ::exit(EXIT_SUCCESS);
        }

        // PARENT
        if(pid < 0)
        {
            cleanup();
            throw std::runtime_error("Failed to fork");
        }

        int status = 0;
        ::waitpid(pid, &status, 0);

        std::array<char, 256> buffer;

        ssize_t bytes = 0;
        do
        {
            bytes = ::read(outfd[READ_END], buffer.data(), buffer.size());
            StdOut.append(buffer.data(), bytes);
        }
        while(bytes > 0);

        do
        {
            bytes = ::read(errfd[READ_END], buffer.data(), buffer.size());
            StdErr.append(buffer.data(), bytes);
        }
        while(bytes > 0);

        if(WIFEXITED(status))
        {
            ExitStatus = WEXITSTATUS(status);
        }

        cleanup();
    }
};

std::string make_daytime_string()
{
    using namespace std; // For time_t, time and ctime;
    time_t now = time(0);
    return ctime(&now);
}

class tcp_connection
        : public boost::enable_shared_from_this<tcp_connection>
{
public:
    typedef boost::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_service& io_service)
    {
        return pointer(new tcp_connection(io_service));
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        output = "Connected to Telnet server!\n";
        write();
        read();
        //output = make_daytime_string();
        //write();
    }

private:
    tcp_connection(boost::asio::io_service& io_service)
            : socket_(io_service)
    {
    }

    void write()
    {
        boost::asio::async_write(socket_, boost::asio::buffer(output),
                                 boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }

    void read()
    {
        boost::asio::async_read_until(socket_, input, "\n", boost::bind(&tcp_connection::handle_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(boost::system::error_code error, std::size_t bytes_transferred)
    {
        if (!error)
        {
            std::istream stream(&input);
            std::string line;
            std::getline(stream, line);
            input.consume(bytes_transferred);
            boost::algorithm::trim(line);
            if (!line.empty())
            {
                Command cmd;
                cmd.Command = line;
                cmd.execute();
                output = cmd.StdOut;
                write();
            }
            read();
        }
    }


    void handle_write(const boost::system::error_code& /*error*/,
                      size_t /*bytes_transferred*/)
    {
    }

    char recv_str[1024];
    boost::asio::ip::tcp::socket socket_;
    std::string output;
    boost::asio::streambuf input;

};

class tcp_server
{
public:
    tcp_server(boost::asio::io_service& io_service)
            : io_service_(io_service),
              acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 3030))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        tcp_connection::pointer new_connection =
                tcp_connection::create(io_service_);

        acceptor_.async_accept(new_connection->socket(),
                               boost::bind(&tcp_server::handle_accept, this, new_connection,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(tcp_connection::pointer new_connection,
                       const boost::system::error_code& error)
    {
        if (!error)
        {
            new_connection->start();
        }

        start_accept();
    }

    boost::asio::io_service& io_service_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

int main()
{
    try
    {
        boost::asio::io_service io_service;
        tcp_server server(io_service);
        io_service.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
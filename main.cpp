#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <sys/wait.h>
#include <map>
#include <charconv>

class Command
{
public:
    int             ExitStatus = 0;
    std::string     Command;
    std::string     StdOut;
    std::string     StdErr;

    void execute()
    {
        StdOut = "";
        StdErr = "";

        const int READ_END = 0;
        const int WRITE_END = 1;

        int infd[2] = {0, 0};
        int outfd[2] = {0, 0};
        int errfd[2] = {0, 0};

        std::array<char, 256> buffer;

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

std::string show_time()
{
    std::time_t now = std::time(0);
    return std::ctime(&now);
}

class tcp_connection
        : public boost::enable_shared_from_this<tcp_connection>
{
public:
    typedef boost::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_context& io_context)
    {
        return pointer(new tcp_connection(io_context));
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        outputline = "Connected to Telnet server!\nUse \"!help\" for help\n\n";
        write();
        read();
    }

private:
    tcp_connection(boost::asio::io_context& io_context)
            : socket_(io_context)
    {
    }

    void write()
    {
        boost::asio::async_write(socket_, boost::asio::buffer(outputline),
                                 boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred));
    }

    void read()
    {
        boost::asio::async_read_until(socket_, input, "\n",
                                      boost::bind(&tcp_connection::handle_read, shared_from_this(),
                                                  boost::asio::placeholders::error,
                                                  boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(boost::system::error_code error,
                     std::size_t bytes_transferred)
    {
        if (!error)
        {
            std::istream stream(&input);
            std::getline(stream, inputline);
            input.consume(bytes_transferred);
            boost::algorithm::trim(inputline);
            if (!inputline.empty())
            {
                commands["!help"] = 1;
                commands["!date"] = 2;
                commands["!stop"] = 3;
                switch(commands[inputline]){
                    case 1:
                        outputline = "You can call shell command or use one of this:\n!date - Print time and date\n!stop - Close connection\n\n";
                        write();
                        break;
                    case 2:
                        outputline = show_time() + "\n";
                        write();
                        break;
                    case 3:
                        stop();
                        break;
                    default:
                        cmd.Command = inputline;
                        cmd.execute();
                        if (cmd.ExitStatus == 0){
                            outputline = "Success!\n\n";
                            write();
                            outputline = cmd.StdOut + "\n";
                            write();
                        }
                        else{
                            outputline = "Something went wrong\n\n";
                            write();
                            outputline = cmd.StdErr + "\n";
                            write();
                        }
                        break;
                }

            }
            read();
        }
    }


    void handle_write(const boost::system::error_code& /*error*/,
                      std::size_t /*bytes_transferred*/)
    {
    }

    void stop()
    {
        socket_.close();
    }
    std::map <std::string, int> commands;
    Command cmd;
    boost::asio::ip::tcp::socket socket_;
    std::string inputline;
    std::string outputline;
    boost::asio::streambuf input;

};

class tcp_server
{
public:
    tcp_server(boost::asio::io_context& io_context, int port)
            : io_context_(io_context),
              acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        tcp_connection::pointer new_connection =
                tcp_connection::create(io_context_);

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

    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
};

int main(int argc, char* argv[])
{
    int port;
    int number_of_treads;
    std::from_chars<int>(argv[1],argv[2], port);
    std::from_chars<int>(argv[2],argv[3], number_of_treads);
    try
    {
        boost::asio::io_context io_context;
        tcp_server server(io_context, port);
        std::vector<std::thread> threads;
        threads.reserve(number_of_treads - 1);
        for(int i = 1; i < number_of_treads; ++i) {
            threads.emplace_back([&io_context]{
                io_context.run();
            });
            //std::cout << "Added thread\n";
        }
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}
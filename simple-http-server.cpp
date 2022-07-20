#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/awaitable.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/use_awaitable.hpp>
#include <boost/asio/redirect_error.hpp>
#include <boost/asio/signal_set.hpp>
#include <filesystem>
#include <iostream>

namespace {

namespace net = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;

using net::use_awaitable;
using net::redirect_error;
using boost::asio::co_spawn;
using boost::asio::detached;
using string_request = beast::http::request<beast::http::string_body>;
using string_response = beast::http::response<beast::http::string_body>;

// Return a reasonable mime type based on the extension of a file.
beast::string_view mime_type(const fs::path& path)
{
    using beast::iequals;
    auto const ext = path.extension().string();
    if(iequals(ext, ".htm"))  return "text/html";
    if(iequals(ext, ".html")) return "text/html";
    if(iequals(ext, ".php"))  return "text/html";
    if(iequals(ext, ".css"))  return "text/css";
    if(iequals(ext, ".txt"))  return "text/plain";
    if(iequals(ext, ".js"))   return "application/javascript";
    if(iequals(ext, ".json")) return "application/json";
    if(iequals(ext, ".xml"))  return "application/xml";
    if(iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if(iequals(ext, ".flv"))  return "video/x-flv";
    if(iequals(ext, ".png"))  return "image/png";
    if(iequals(ext, ".jpe"))  return "image/jpeg";
    if(iequals(ext, ".jpeg")) return "image/jpeg";
    if(iequals(ext, ".jpg"))  return "image/jpeg";
    if(iequals(ext, ".gif"))  return "image/gif";
    if(iequals(ext, ".bmp"))  return "image/bmp";
    if(iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if(iequals(ext, ".tiff")) return "image/tiff";
    if(iequals(ext, ".tif"))  return "image/tiff";
    if(iequals(ext, ".svg"))  return "image/svg+xml";
    if(iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// Returns a bad request response
auto ok_response (const string_request& req, beast::string_view message)
{
    string_response res {http::status::ok, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(message);
    res.prepare_payload();

    return res;
};

// Returns a bad request response
auto bad_request (const string_request& req, beast::string_view reason)
{
    string_response res {http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = std::string(reason);
    res.prepare_payload();

    return res;
};

// Returns a not found response
auto not_found (const string_request& req, beast::string_view target)
{
    string_response res {http::status::not_found, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "The resource '" + std::string(target) + "' was not found.";
    res.prepare_payload();

    return res;
};

// Returns a server error response
auto server_error (const string_request& req, beast::string_view reason)
{
    string_response res {http::status::internal_server_error, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = "An error occurred: '" + std::string(reason) + "'";
    res.prepare_payload();

    return res;
};

// Coroutine to serialize and send a string response and return the error code
net::awaitable<beast::error_code> send_string_response(
    net::ip::tcp::socket& socket,
    const string_request& req,
    const string_response& resp)
{
    beast::error_code ec;
    http::response_serializer<http::string_body> srs{resp};
    co_await http::async_write(socket, srs, redirect_error(use_awaitable, ec));

    co_return ec;
}

// Session coroutine is run for each client
net::awaitable<void> session(
    net::ip::tcp::socket socket,
    beast::string_view document_root, 
    beast::string_view default_document)
{
    try
    {
        beast::error_code ec;
        beast::flat_buffer buffer;

        for (;;)
        {
            string_request req;
            co_await http::async_read(socket, buffer, req, redirect_error(use_awaitable, ec));

            if(ec == http::error::end_of_stream)
            {
                // Connection closed.
                break;
            }
            else if (ec)
            {
                std::cout << "HTTP read error: " << ec.what() << std::endl;
                break;
            }

            // Make sure we can handle the method
            if (req.method() != http::verb::get && req.method() != http::verb::head)
            {
                co_await send_string_response(socket, req, bad_request(req, "Unknown HTTP-method"));
                break;
            }

            if (req.target().empty() ||
                req.target()[0] != '/' ||
                req.target().find("..") != beast::string_view::npos)
            {
                co_await send_string_response(socket, req, bad_request(req, "Illegal request-target"));
                break;
            }

            // Build the path to the requested file
            fs::path target_path {req.target().cbegin() + 1, req.target().cend()};
            if (target_path.empty())
            {
                target_path = fs::path{default_document.cbegin(), default_document.cend()};
            }

            fs::path root_path {document_root.cbegin(), document_root.cend()};
            auto path = root_path / target_path;

            // Attempt to open the file
            http::file_body::value_type body;
            body.open(path.c_str(), beast::file_mode::scan, ec);

            // Handle the case where the file doesn't exist
            if(ec == beast::errc::no_such_file_or_directory)
            {
                co_await send_string_response(socket, req, not_found(req, req.target()));
                break;
            }

            // Handle an unknown error
            if(ec)
            {
                co_await send_string_response(socket, req, server_error(req, ec.message()));
                break;
            }

            // Cache the size since we need it after the move
            auto const size = body.size();

            // Respond to HEAD request
            if(req.method() == http::verb::head)
            {
                http::response<http::empty_body> res{http::status::ok, req.version()};
                res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
                res.set(http::field::content_type, mime_type(path));
                res.content_length(size);
                res.keep_alive(req.keep_alive());
                http::response_serializer<http::empty_body> srs{res};
                co_await http::async_write(socket, srs, redirect_error(use_awaitable, ec));
            }

            // Respond to GET request
            http::response<http::file_body> res{
                std::piecewise_construct,
                std::make_tuple(std::move(body)),
                std::make_tuple(http::status::ok, req.version())};
            res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
            res.set(http::field::content_type, mime_type(path.string()));
            res.content_length(size);
            res.keep_alive(req.keep_alive());
            http::response_serializer<http::file_body> srs{res};
            co_await http::async_write(socket, srs, redirect_error(use_awaitable, ec));
        }
    }
    catch (std::exception& e)
    {
        std::printf("echo Exception: %s\n", e.what());
    }
}

net::awaitable<void> http_server(
    const std::string& address,
    const std::string& port,
    beast::string_view document_root,
    boost::string_view default_document)
{
    namespace this_coro = boost::asio::this_coro;

    auto executor = co_await this_coro::executor;
    net::ip::tcp::resolver resolver {executor};
    net::ip::tcp::endpoint endpoint = *resolver.resolve(address, port).begin();
	net::ip::tcp::acceptor acceptor {executor, endpoint};

    std::cout
        << "HTTP server is listening on "
        << acceptor.local_endpoint().address().to_string() << ":"
        << acceptor.local_endpoint().port() << std::endl;

    for (;;)
    {
        net::ip::tcp::socket socket = co_await acceptor.async_accept(use_awaitable);
        co_spawn(executor, session(std::move(socket), document_root, default_document), detached);
    }
}

} // namespace

int main(int argc, char* argv[])
{
	if (argc < 4)
	{
		std::cerr << "Usage: simple-http-server address port doc_root [default_document]\n";
		return 1;
	}

    try
    {
        // Application main io context
        boost::asio::io_context io_context;

        // Register to handle the signals that indicate when the server should exit.
        boost::asio::signal_set signals {io_context, SIGINT, SIGTERM, SIGQUIT};
        signals.async_wait([&](const boost::system::error_code&, int) {
            std::cout << "Interrupted.\nExiting." << std::endl;
            io_context.stop();
        });

        std::string default_document = argc > 4 ? argv[4] : "index.html";

        // Spawn the http server.
        boost::asio::co_spawn(
            io_context,
            http_server(argv[1], argv[2], argv[3], default_document),
            boost::asio::detached);

        // Run the io context
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << '\n';
		return 1;
    }
    catch (...)
    {
        std::cerr << "Unknown error.\n";
		return 1;
    }

	return 0;
}

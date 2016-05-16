#define BOOST_FILESYSTEM_NO_DEPRECATED
#define GCC_VERSION (__GNUC__ * 10000 \
                               + __GNUC_MINOR__ * 100 \
                               + __GNUC_PATCHLEVEL__)
/* Test for GCC > 3.2.0 */
#if GCC_VERSION  < 50300
#define BOOST_NO_CXX11_SCOPED_ENUMS
#endif

#include <boost/filesystem.hpp>
#include "fs_manager.h"
#include <sstream>
#include <cassert>
#include <iostream>
#include "io/locales.h"

namespace cynny {
namespace cynnypp {
namespace filesystem {

using boost::filesystem::file_type;
using namespace impl;


// definition of the "tag" for FilesystemError messages
const std::string ErrorCode::tag{"FilesystemError: "};


std::string ErrorCode::what() const
{
    std::ostringstream os(tag);
    os << "error " << val << ": " << msg;
    return os.str();
}

// -----------------------------------------------------------------------------------------------
// is_admitted: check whether a file_type is among those specified in the template parameters pack
// -----------------------------------------------------------------------------------------------
template<boost::filesystem::file_type Admitted>
static constexpr bool is_admitted(boost::filesystem::file_type t)
{
    return t == Admitted;
}

template<boost::filesystem::file_type Head, boost::filesystem::file_type... Tail>
using enable_if_t = typename std::enable_if<sizeof...(Tail) != 0, bool>::type;

template<boost::filesystem::file_type Head, boost::filesystem::file_type... Tail>
static constexpr enable_if_t<Head, Tail...> is_admitted(boost::filesystem::file_type t)
{
    return t == Head || is_admitted<Tail...>(t);
}


// -----------------------------------------------------------------------------------------------
// check_path_admitted: check whether the type of the path is among the admitted ones
// -----------------------------------------------------------------------------------------------
template<boost::filesystem::file_type... Admitted>
static boost::filesystem::file_status check_path_admitted(const boost::filesystem::path& p)
{
    using boost::filesystem::file_type;

    try {
        auto status = boost::filesystem::symlink_status(p);
        auto type = status.type();

        // admit only file_not_found, regular_file or directory_file
        if(!is_admitted<Admitted...>(type))
        {
            std::string msg = std::string("path \"") + p.native() + "\" is not admitted";
            throw ErrorCode(ErrorCode::invalid_argument, msg);
        }

        return status;
    }
    // capture boosts exceptions to rethrow a FilesystemError
    catch(boost::filesystem::filesystem_error& e)
    {
        throw ErrorCode(ErrorCode::internal_failure, e.what());
    }
}



// -----------------------------------------------------------------------------------------------
// standalone helper functions for filesytem synchronous I/O operations
// -----------------------------------------------------------------------------------------------

bool FilesystemManager::exists_static(const Path& p)
{
    // check and throw if needed
    auto status = check_path_admitted<file_type::file_not_found,file_type::regular_file,file_type::directory_file>(p);
    try {
        return boost::filesystem::exists(status);
    }
    catch (const boost::filesystem::filesystem_error& e) {
        throw ErrorCode(ErrorCode::internal_failure, e.what());
    }
}


bool FilesystemManager::removeFile_static(const Path& p)
{
    boost::filesystem::path boost_path{p};
    // check and throw if needed
    check_path_admitted<file_type::file_not_found,file_type::regular_file>(boost_path);
    try {
        return boost::filesystem::remove(boost_path);
    }
    catch (const boost::filesystem::filesystem_error& e) {
        throw ErrorCode(ErrorCode::internal_failure, e.what());
    }
}

static void move_(const boost::filesystem::path& from, const boost::filesystem::path& to)
{
    // if the destination path already exists and is a directory, I must emulate "mv" command behavior
    if(boost::filesystem::is_directory(to))
        return move_(from, to / *--from.end());

    try {
        boost::filesystem::rename(from, to);
    }
    catch (const boost::filesystem::filesystem_error& e) {
        throw ErrorCode(ErrorCode::internal_failure, e.what());
    }
}


void FilesystemManager::move_static(const Path &from, const Path &to)
{
    boost::filesystem::path boost_from{from};
    boost::filesystem::path boost_to{to};

    // check and throw if needed
    check_path_admitted<file_type::regular_file,file_type::directory_file>(boost_from);

    move_(boost_from, boost_to);
}


void FilesystemManager::copyFile_static(const Path& from, const Path& to)
{
    if(from == to)
        throw ErrorCode(ErrorCode::operation_not_permitted, "source and destination match");
    boost::filesystem::path boost_from{from};
    boost::filesystem::path boost_to{to};
    // check and throw if needed
    check_path_admitted<file_type::regular_file>(boost_from);
    // if to is a directory, we copy the file inside it
    if(boost::filesystem::is_directory(boost_to) && boost::filesystem::exists(boost_to))
        boost_to /= boost_from.filename();
    try {
        boost::filesystem::copy_file(boost_from, boost_to, boost::filesystem::copy_option::overwrite_if_exists);
    }
    catch (const boost::filesystem::filesystem_error& e) {
        throw ErrorCode(ErrorCode::internal_failure, e.what());
    }
}

/* 
 * Precondition: boost::filesystem::is_directory(from) == true
 */
void copyDirectory_(const boost::filesystem::path& from, const boost::filesystem::path& to_)
{
    try {
        // if "to_" is a directory, we copy the dir and its content inside "from"
        // example: from = a/, to_ = b/, actual_to = a/b/
        const auto actual_from = from / std::string{boost::filesystem::path::preferred_separator};
        const auto actual_to = boost::filesystem::is_directory(to_) ? to_ /  actual_from.parent_path().filename() : to_;

        // boost copy_directory just create a new directory with copied attributes
        boost::filesystem::copy_directory(from, actual_to);
        
        // recursively copy directory content
        boost::filesystem::directory_iterator end_itr;
        for(boost::filesystem::directory_iterator p(from);  p != end_itr; ++p) {
            const auto& sub_to = actual_to / p->path().filename();
            if(boost::filesystem::is_directory(*p))
                copyDirectory_(*p, sub_to);
            else if(boost::filesystem::symlink_status(*p).type() == file_type::regular_file)
                boost::filesystem::copy(*p, sub_to);
        }
    }
    catch (const boost::filesystem::filesystem_error& e) {
        throw ErrorCode(ErrorCode::internal_failure, e.what());
    }
}

void FilesystemManager::copyDirectory_static(const Path& from, const Path& to)
{
    boost::filesystem::path boost_from{from};
    boost::filesystem::path boost_to{to};
    // check and throw if needed
    check_path_admitted<file_type::directory_file>(boost_from);
    copyDirectory_(boost_from, boost_to);
}


uintmax_t FilesystemManager::removeDirectory_static(const Path& p)
{
    boost::filesystem::path boost_path{p};
    // check and throw if needed
    check_path_admitted<file_type::file_not_found,file_type::directory_file>(boost_path);
    try {
        return boost::filesystem::remove_all(boost_path);
    }
    catch (const boost::filesystem::filesystem_error& e) {
        throw ErrorCode(ErrorCode::internal_failure, e.what());
    }
}


bool FilesystemManager::createDirectory_static(const Path &p, bool parents)
{
    boost::filesystem::path boost_path{p};
    if(parents) {
        // check and throw if needed
        check_path_admitted<file_type::file_not_found,file_type::directory_file>(boost_path);
        try {
            return boost::filesystem::create_directories(boost_path);
        }
        catch (const boost::filesystem::filesystem_error& e) {
            throw ErrorCode(ErrorCode::internal_failure, e.what());
        }
    }
    else {
        // check and throw if needed
        check_path_admitted<file_type::file_not_found>(boost_path);
        // if the directory name ends with a "/", applying parent_path will only remove the trailing "/", so I've to call it twice
        auto parent_path = *--boost_path.end() == "." ? boost_path.parent_path().parent_path() : boost_path.parent_path();
        check_path_admitted<file_type::directory_file>(parent_path);
        return boost::filesystem::create_directory(boost_path);
    }
}


// --------------------- file access --------------

Buffer FilesystemManager::readFile_static(const Path& p)
{
    // check and throw if needed
    check_path_admitted<file_type::regular_file>(p);

    std::basic_ifstream<uint8_t> in(p, std::ios::in | std::ios::binary | std::ios::ate);
    if (!in)
        throw(ErrorCode(ErrorCode::open_failure, std::string{__func__} + " was not able to open the file " + p + " in read mode"));

    Buffer ret;
    ret.resize(in.tellg());
    in.seekg(0);
    in.imbue(utilities_locale);
    in.read(ret.data(), ret.size());

    if (!in) {
        std::ostringstream msg(__func__);
        msg << " was not able to read from file " << p << " after reading " << in.gcount() << " characters";
        throw (ErrorCode(ErrorCode::read_failure, msg.str()));
    }

    return ret;
}





void FilesystemManager::writeFile_static(const Path& p, const Buffer& buf)
{
    // check and throw if needed
    check_path_admitted<file_type::file_not_found,file_type::regular_file>(p);

    std::basic_ofstream<uint8_t> os(p, std::ios::binary);
    if(!os)
        throw(ErrorCode(ErrorCode::open_failure, std::string{__func__} + " was not able to open the file " + p + " in write mode"));

    os.imbue(utilities::utilities_locale);
    os.write(buf.data(), buf.size());

    if(!os)
        throw(ErrorCode(ErrorCode::write_failure, std::string{__func__} + " was not able to write to the file " + p));
}




void FilesystemManager::appendToFile_static(const Path& p, const Buffer& bytes)
{
    // check and throw if needed
    auto status = check_path_admitted<file_type::file_not_found,file_type::regular_file>(p);

    auto flags = std::ios::binary | std::ios::out;
    if(boost::filesystem::exists(status))
         flags |= std::ios::ate| std::ios::in ;
    std::basic_fstream<uint8_t> ios(p, flags);
    if(!ios)
        throw(ErrorCode(ErrorCode::open_failure, std::string{__func__} + " was not able to open the file " + p + " in read/write mode"));

    ios.imbue(utilities::utilities_locale);
    ios.write(bytes.data(), bytes.size());

    if(!ios)
        throw(ErrorCode(ErrorCode::append_failure, std::string{__func__} + " was not able to append to file " + p + '\n'));
}


// -----------------------------------------------------------------------------------------------
// standalone helper functions for filesytem synchronous I/O operations
// -----------------------------------------------------------------------------------------------

bool FilesystemManager::exists(const Path& p)
{
    return exists_static(p);
}


bool FilesystemManager::removeFile(const Path& p)
{
    return removeFile_static(p);
}


void FilesystemManager::move(const Path &from, const Path &to)
{
    return move_static(from, to);
}


void FilesystemManager::copyFile(const Path& from, const Path& to)
{
    return copyFile_static(from, to);
}

void FilesystemManager::copyDirectory(const Path& from, const Path& to)
{
    return copyDirectory_static(from, to);
}


uintmax_t FilesystemManager::removeDirectory(const Path& p)
{
    return removeDirectory_static(p);
}


bool FilesystemManager::createDirectory(const Path &p, bool parents)
{
    return createDirectory_static(p, parents);
}


// --------------------- file access --------------

Buffer FilesystemManager::readFile(const Path& p)
{
    return readFile_static(p);
}




void FilesystemManager::writeFile(const Path& p, const Buffer& buf)
{
    return writeFile_static(p, buf);
}


void FilesystemManager::appendToFile(const Path& p, const Buffer& bytes)
{
    return appendToFile_static(p, bytes);
}




void FilesystemManager::async_read(const Path& p, Buffer& buf, CompletionHandler h)
{
    // enque a read request to the waiting queue
    q_.push_read(p, buf, std::move(h));
    available_.set_event();
}


void FilesystemManager::async_write(const Path& p, const Buffer& buf, CompletionHandler h)
{
    // enqueue a write request to the waiting queue
    q_.push_write(p, buf, std::move(h));
    available_.set_event();
}



void FilesystemManager::async_append(const Path&p, const Buffer &buf, FilesystemManagerInterface::CompletionHandler h) {
    q_.push_append(p, buf, std::move(h));
    available_.set_event();
}


// -----------------------------------------------------------------------------------------------------

/*
 * class FilesystemManager implementation
 */

FilesystemManager::FilesystemManager(boost::asio::io_service& io)
: io_(io)
, done_(false)
, thrd_(FilesystemManager::process_queue,std::ref(*this))
{}

void FilesystemManager::process_queue(FilesystemManager& fs)
{
    // while the object isn't destroyed/stopped
    while(!fs.done_) {
        // wait on the queue for an operation to be available
        fs.available_.wait_event();
        fs.available_.reset_event();
        while(!fs.q_.empty()) {
            // execute the operation
            fs.perform_next_operation();
        }
    }
}

FilesystemManager::~FilesystemManager()
{
    // set the completion flag to true, notify the event and wait for the working thread to finish
    done_ = true;
    available_.set_event();
    thrd_.join();
}


void FilesystemManager::perform_next_operation()
{
    using std::get;

    CompletionHandler h;
    ErrorCode ec{ErrorCode::unknown_error};
    size_t size{0};

    try {
        switch (q_.front()) {
        case OperationCode::async_read: {
            std::tuple<OperationCode,const Path,std::reference_wrapper<Buffer>,CompletionHandler> t = q_.pop_read();
            std::reference_wrapper<Buffer> tmp = std::get<2>(t);
            auto &buf = tmp.get();
            h = std::move(std::get<3>(t));
            buf = readFile_static(std::get<1>(t));
            ec = ErrorCode::success;
            size = buf.size();
        }
            break;
        case OperationCode::async_write: {
            std::tuple<OperationCode,const Path,std::reference_wrapper<const Buffer>,CompletionHandler> t = q_.pop_write();
            const auto& buf = get<2>(t).get();
            h = std::move(get<3>(t));
            writeFile_static(get<1>(t), buf);
            ec = ErrorCode::success;
            size = buf.size();
        }
            break;
        case OperationCode::async_append: {
            auto t = q_.pop_write();
            const auto &buf = get<2>(t).get();
            h = std::move(get<3>(t));
            appendToFile_static(get<1>(t), buf);
            ec = ErrorCode::success;
            size = buf.size();
        } break;
        case OperationCode::async_read_chunk: {
            std::tuple<OperationCode,std::shared_ptr<impl::ChunkedReader>,size_t,impl::HotDoubleBuffer::BufferView,CompletionHandler>  t = q_.pop_chunked_read();
            auto pos = get<2>(t);
            h = std::move(get<4>(t));
            auto reader = get<1>(t);
            // check that the reader has not been stopped
            if(reader->is_stopped()) {
                ec = ErrorCode::stopped;
                break;
            }
            // check that the number of buff
            auto buf = get<3>(t);
            // if this is not the next chunk to be read or the buffer is full
            if(reader->bytes_read() != pos || buf.is_hot()) {
                // enque the current operation at the end
                q_.push_chunked_read(reader, pos, buf, std::move(h));
                return;
            }

            auto size_to_read = buf.size();
            reader->read_file_chunk(buf, buf.size(), pos);
            buf.set_hot(true);
            ec = buf.error_code() = (buf.size() == size_to_read) & !reader->eof() ? ErrorCode::success : ErrorCode::end_of_file;
            size = buf.size();
        }
            break;
        default:
            assert(0);
            break;
        }
    }
    catch(const ErrorCode& e) {
        ec = e;
    }

    // post the completion handler to the boost asio io_service
    io_.post(std::bind(std::move(h), ec, size));
}


std::shared_ptr<ChunkedFstreamInterface> FilesystemManager::make_chunked_stream(const Path& p, size_t chunk_size)
{
    // check and throw if needed
    check_path_admitted<file_type::regular_file>(p);

    if(chunk_size == 0)
        throw ErrorCode(ErrorCode::invalid_argument, "[ERROR] FilesystemManager: can only read in chunks of size > 0.");

    // workaround that enable to keep DownloadActivity's constructor protected (see http://stackoverflow.com/a/25069711/2508150)
    struct make_shared_enabler : ChunkedFstream { make_shared_enabler(FilesystemManager& fs, std::shared_ptr<impl::ChunkedReader> r) : ChunkedFstream(fs, r) {} };
    return std::make_shared<make_shared_enabler>(*this, std::make_shared<ChunkedReader>(*this, p, chunk_size));
}

ChunkedReader::ChunkedReader(FilesystemManager& fs, const Path& p, size_t chunk_size)
    : fs_manager(fs)
    , path(p)
    , is(p)
    , file_size((is.seekg(0, std::ios::end), is.tellg()))
    , pos_to_schedule(0)
    , bytes_read_(0)
    , n_enqueued(0)
    , buf_(chunk_size)
    , stopped(false)
{
    if(!is)
        throw ErrorCode(ErrorCode::open_failure, "ChunkedReader was not able to open the file");

    // set uint8_t locale (to be able to read using uint8_t type)
    is.imbue(utilities::utilities_locale);
    // seek to the beginning (we've seeked to the end to check file size)
    is.seekg(0, std::ios::beg);
    // get the version
}



void ChunkedReader::next_chunk(ReadChunkHandler h)
{
    if(stopped) {
        fs_manager.get_io_service().post(std::bind(std::move(h), ErrorCode::stopped, Buffer{}));
        return;
    }

    if(!q_buf_ready.empty()) {
        assert(q_buf_ready.front().is_hot());
        Buffer buf_copy{static_cast<const Buffer &>(q_buf_ready.front())};
        q_buf_ready.front().set_hot(false);
        h(q_buf_ready.front().error_code(), std::move(buf_copy));
        q_buf_ready.pop();
    }
    else {
        if(n_enqueued > q_handlers.size())
            q_handlers.push(std::move(h));
        else {
            if(pos_to_schedule >= file_size) {
                auto error_code = ErrorCode::end_of_file;
                fs_manager.get_io_service().post(std::bind(std::move(h), error_code, Buffer{}));
                return;
            }
            auto buf_curr = buf_.get_and_swap();
            auto shared = shared_from_this();
            fs_manager.async_read_chunk(shared_from_this(), pos_to_schedule, buf_curr, [buf_curr, shared, h](const ErrorCode& ec,  size_t bytesRead) mutable
            {
                assert(buf_curr.is_hot());
                Buffer buf_copy{static_cast<Buffer>(buf_curr)};
                buf_curr.set_hot(false);
                h(ec, std::move(buf_copy));
            });
            pos_to_schedule += buf_curr.size();
        }
    }

    if(n_enqueued < 2) { // 2 is because we are using a double buffer for prefetching
        if(pos_to_schedule >= file_size) {
            //if I've read enough, I shall not prefetch anymore
            return;
        }
        // schedule on the FilesystemManager the read of the next chunk
        auto buf_curr = buf_.get_and_swap();
        auto shared = shared_from_this();
        fs_manager.async_read_chunk(shared_from_this(), pos_to_schedule, buf_curr, [shared, buf_curr](const ErrorCode& ec, size_t l) mutable
        {
            if(!shared->q_handlers.empty()) {
                auto enqueued_h = std::move(shared->q_handlers.front());
                shared->q_handlers.pop();
                assert(buf_curr.is_hot());
                Buffer buf_copy{static_cast<Buffer>(buf_curr)};
                buf_curr.set_hot(false);
                enqueued_h(ec, std::move(buf_copy));
                --shared->n_enqueued;
            }
            else {
                shared->q_buf_ready.push(buf_curr);
            }
        });
        ++n_enqueued;
        pos_to_schedule += buf_curr.size();
    }
}

Buffer ChunkedReader::read_file_chunk(HotDoubleBuffer::BufferView &buf, size_t chunk_size, size_t pos)
{
    if (!is)
        // NOTE: error code rather arbitrary... can we find a better code?
        throw(ErrorCode(ErrorCode::read_failure, std::string{__func__} + ": the stream supplied wai in fail state\n"));

    buf.resize(chunk_size);
    is.seekg(pos);
    is.read(buf.data(), buf.size());
    // resize the return buffer to the number of bytes effectively read
    buf.resize(is.gcount());
    bytes_read_ += buf.size();

    return buf;
}

void FilesystemManager::OperationsQueue::push_read(const Path &path, Buffer &buf, FilesystemManager::CompletionHandler h)
{
    std::lock_guard<std::mutex> lck{mtx};
    q_operations.emplace(OperationCode::async_read, std::move(h));
    q_read_data.emplace(path, buf);
}

void FilesystemManager::OperationsQueue::push_write(const Path &path, const Buffer &buf, FilesystemManager::CompletionHandler h)
{
    std::lock_guard<std::mutex> lck{mtx};
    q_operations.emplace(OperationCode::async_write, std::move(h));
    q_write_data.emplace(path, buf);
}

void FilesystemManager::OperationsQueue::push_append(const Path &path, const Buffer &buf, FilesystemManager::CompletionHandler h) {
    q_operations.emplace(OperationCode::async_append, std::move(h));
    q_write_data.emplace(path, buf);
};

void FilesystemManager::OperationsQueue::push_chunked_read(std::shared_ptr<ChunkedReader> r, size_t pos, HotDoubleBuffer::BufferView &buf, FilesystemManager::CompletionHandler h)
{
    std::lock_guard<std::mutex> lck{mtx};
    q_operations.emplace(OperationCode::async_read_chunk, std::move(h));
    q_as_read_data.emplace(r, pos, buf);
}

std::tuple<FilesystemManager::OperationCode,const Path,std::reference_wrapper<Buffer>,FilesystemManager::CompletionHandler> FilesystemManager::OperationsQueue::pop_read()
{
    using std::get;
    std::lock_guard<std::mutex> lck{mtx};
    auto op = std::move(q_operations.front());
    q_operations.pop();
    std::tuple<const Path,std::reference_wrapper<Buffer>> path_buf = std::move(q_read_data.front());
    q_read_data.pop();
    return std::make_tuple(op.first, std::move(get<0>(path_buf)), get<1>(path_buf), std::move(op.second));
}

std::tuple<FilesystemManager::OperationCode,const Path, std::reference_wrapper<const Buffer>,FilesystemManager::CompletionHandler> FilesystemManager::OperationsQueue::pop_write()
{
    using std::get;
    std::lock_guard<std::mutex> lck{mtx};
    auto op = std::move(q_operations.front());
    q_operations.pop();
    std::tuple<const Path,std::reference_wrapper<const Buffer>> path_buf = std::move(q_write_data.front());
    q_write_data.pop();
    return std::make_tuple(op.first, std::move(get<0>(path_buf)), get<1>(path_buf), std::move(op.second));
}



std::tuple<FilesystemManager::OperationCode,std::shared_ptr<ChunkedReader>,size_t,HotDoubleBuffer::BufferView,FilesystemManager::CompletionHandler> FilesystemManager::OperationsQueue::pop_chunked_read()
{
    using std::get;
    std::lock_guard<std::mutex> lck{mtx};
    auto op = std::move(q_operations.front());
    q_operations.pop();
    std::tuple<std::shared_ptr<impl::ChunkedReader>,size_t,impl::HotDoubleBuffer::BufferView> reader = std::move(q_as_read_data.front());
    q_as_read_data.pop();
    return std::make_tuple(op.first, get<0>(reader), get<1>(reader), std::ref(get<2>(reader)), std::move(op.second));
}

// --------------------------------------------- chunked fstream

ChunkedFstream::~ChunkedFstream()
{
    reader->stop();
}

void ChunkedFstream::next_chunk(FilesystemManager::ReadChunkHandler h)
{
    reader->next_chunk(std::move(h));
}

} // namespace filesystem
} // namespace cynnypp
} // namespace cynny

template <uint64_t T>
void DiffLogic<T>::generate_frame(uint64_t type, const std::string & message, uint32_t mask_key)
{
    Frame frame;
    frame.fin      = true;
    frame.type     = type;
    frame.mask_key = mask_key;
    frame.masked   = mask_key;

    uint64_t size = message.size();
    auto fr = frame.serialize(message.empty() ? nullptr : (const uint8_t *)message.data(), size);
    m_output_queue.emplace_back(fr, size);
}

template <uint64_t T>
void DiffLogic<T>::on_read(Socket *socket)
{
    int64_t readed = 0;

    while ((readed = socket->read(m_buffer + m_offset, BUFFER_SIZE - m_offset)))
    {
        m_offset += readed;

        if (m_state == HANDSHAKE_RESPONSE)
        {
            auto delim = strstr((char *)m_buffer, "\r\n\r\n");
            if (!delim || delim - (char *)m_buffer > m_offset)
                continue;

            ::memcpy(m_buffer, delim + 4, m_offset - (delim + 4 - (char *)m_buffer));
            m_offset -= (delim + 4 - (char *)m_buffer);

            m_state = IN_WORK;

            m_offset = 0;

            generate_frame(Frame::TEXT, std::string(R"rrr({"event":"pusher:subscribe","data":{"channel":"diff_order_book"}})rrr"));
        }

        Frame frame;
        auto end = frame.deserialize(m_buffer, m_offset);
        if (end == m_buffer)
            continue;

        if (frame.type == Frame::PING)
            generate_frame(Frame::PONG, std::string());

        Json::Value root;
        Json::Reader reader;
        bool res = reader.parse((const char *)frame.payload, (const char *)end, root );
        if (!res)
        {
            std::cerr << "invalid json: " << std::string((const char *)frame.payload, end - frame.payload) << std::endl;

            ::memcpy(m_buffer, end, m_offset - (end - m_buffer));
            m_offset -= end - m_buffer;

            continue;
        }

        auto json_data = root["data"];
        if (!json_data)
            assert(false);

        Json::Value data;
        res = reader.parse(json_data.asString(), data);
        if (!res)
        {
            std::cerr << "invalid json" << std::endl;
            assert(false);
        }

#define STORE_ORDER(type) do \
                          { \
                             price = std::stod(level[0].asString()) * 1000000; \
                             amount = std::stod(level[1].asString()) * 1000000; \
                             m_orders.add_order(type, timestamp, price, amount); \
                          } \
                          while (0);


        auto json_timestamp = data["timestamp"];
        if (!!json_timestamp)
        {
            uint64_t timestamp = std::stoll(json_timestamp.asString());
            double price = 0;
            double amount = 0;

            auto bids = data["bids"];
            for (const auto level : bids)
                STORE_ORDER(Order::BUY);

            auto asks = data["asks"];
            for (const auto level : asks)
                STORE_ORDER(Order::SELL);
        }
        ::memmove(m_buffer, end, m_offset - (end - m_buffer));
        m_offset -= end - m_buffer;
    }

#undef STORE_ORDER
}

template <uint64_t T>
void DiffLogic<T>::write_handshake(Socket *socket)
{
    std::string request;

    request = "GET " + m_url.url() + " HTTP/1.1\r\n";
    request += "Host: " + m_url.host() + "\r\n";
    request += "Origin: " + m_url.host() + "\r\n";
    request += "Upgrade: websocket\r\n";
    request += "Connection: Upgrade\r\n";
    request += "Sec-WebSocket-Version: 13\r\n";
    request += "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";

    socket->write((const uint8_t *)request.data(), request.size());
    m_state = HANDSHAKE_RESPONSE;

}

template <uint64_t T>
void DiffLogic<T>::on_write(Socket *socket)
{
    if (m_state == HANDSHAKE)
    {
        write_handshake(socket);
        return;
    }

    int64_t writed = 0;
    auto first_frame = m_output_queue.begin();
    while (!m_output_queue.empty() &&
           (writed = socket->write(first_frame->frame.get() + first_frame->offset, first_frame->size - first_frame->offset)))
    {
        first_frame->offset += writed;
        if (first_frame->offset == first_frame->size)
        {
            m_output_queue.pop_front();
            first_frame = m_output_queue.begin();
        }
    }
}

template <uint64_t T>
void DiffLogic<T>::on_close(Socket *socket, const uint64_t &)
{
}

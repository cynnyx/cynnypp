#include "locales.h"


/**
 * \cond doxygen should ignore this line
 */
std::locale::id std::codecvt<std::uint8_t,char,std::mbstate_t>::id;
/**
 * \endcond
 */

// -----------------------------------------------------------------------------------------------
// Implementation of virtual functions of std::codecvt specialization
// -----------------------------------------------------------------------------------------------
std::codecvt_base::result std::codecvt<uint8_t,char,std::mbstate_t>::do_out(state_type& , const internal_type* from, const internal_type* , const internal_type*& from_next, external_type* to, external_type* , external_type*& to_next) const
{
    from_next = from;
    to_next = to;
   return codecvt_base::noconv;
}

std::codecvt_base::result std::codecvt<uint8_t,char,std::mbstate_t>::do_in(state_type& , const external_type* from, const external_type* , const external_type*& from_next, internal_type* to, internal_type* , internal_type*& to_next) const
{
    from_next = from;
    to_next = to;
    return std::codecvt_base::noconv;
}

std::codecvt_base::result std::codecvt<uint8_t,char,std::mbstate_t>::do_unshift(state_type& , external_type* to, external_type* , external_type*& to_next) const
{
    to_next = to;
    return std::codecvt_base::noconv;
}

int std::codecvt<uint8_t,char,std::mbstate_t>::do_length(state_type& , const external_type* from, const external_type* from_end, std::size_t max) const
{
    return static_cast<int>(std::min< std::size_t >(max, static_cast<std::size_t>(from_end - from)));
}

int std::codecvt<uint8_t,char,std::mbstate_t>::do_max_length() const noexcept
{
    return 1;
}

int std::codecvt<uint8_t,char,std::mbstate_t>::do_encoding() const noexcept
{
    return 1;
}

bool std::codecvt<uint8_t,char,std::mbstate_t>::do_always_noconv() const noexcept
{
    return true;
}


// -----------------------------------------------------------------------------------------------
//                                  fs_locale
// -----------------------------------------------------------------------------------------------

// definition of the locale to be used by fs_manager; the "character" type is uint8_t
const std::locale cynny::cynnypp::utilities::utilities_locale(std::locale{}, new std::codecvt<uint8_t, char, std::mbstate_t>);

#ifndef SMUGGLER_LOCALES_H
#define SMUGGLER_LOCALES_H

#include <locale>


// -----------------------------------------------------------------------------------------------
//   KEEP CALM: specialization inside namespace std
// -----------------------------------------------------------------------------------------------
namespace std{

// -----------------------------------------------------------------------------------------------
// Specialization of std::codecvt to work with uint8_t
// -----------------------------------------------------------------------------------------------
template<>
class codecvt<uint8_t,char,std::mbstate_t > : public locale::facet, public codecvt_base
{
public:
    using internal_type = uint8_t;
    using external_type = char;
    using state_type = std::mbstate_t;

    static std::locale::id id;

    codecvt(std::size_t refs = 0)
        : locale::facet(refs)
    {}

    result out( state_type& state,
                const internal_type* from,
                const internal_type* from_end,
                const internal_type*& from_next,
                external_type* to,
                external_type* to_end,
                external_type*& to_next ) const
    {
        return do_out(state, from, from_end, from_next, to, to_end, to_next);
    }

    result in( state_type& state,
               const external_type* from,
               const external_type* from_end,
               const external_type*& from_next,
               internal_type* to,
               internal_type* to_end,
               internal_type*& to_next ) const
    {
        return do_in(state, from, from_end, from_next, to, to_end, to_next);
    }

    result unshift( state_type& state,
                    external_type* to,
                    external_type* to_end,
                    external_type*& to_next) const
    {
        return do_unshift(state, to, to_end, to_next);
    }

    int encoding() const
    {
        return do_encoding();
    }

    bool always_noconv() const
    {
        return do_always_noconv();
    }

    int length( state_type& state,
                const external_type* from,
                const external_type* from_end,
                std::size_t max ) const
    {
        return do_length(state, from, from_end, max);
    }

    int max_length() const
    {
        return do_max_length();
    }

 protected:
     virtual ~codecvt() {}
     virtual std::codecvt_base::result do_out(state_type& state, const internal_type* from, const internal_type* from_end, const internal_type*& from_next, external_type* to, external_type* to_end, external_type*& to_next) const;
     virtual std::codecvt_base::result do_in(state_type& state, const external_type* from, const external_type* from_end, const external_type*& from_next, internal_type* to, internal_type* to_end, internal_type*& to_next) const;
     virtual std::codecvt_base::result do_unshift(state_type& state, external_type* to, external_type* to_end, external_type*& to_next) const;
     virtual int do_length(state_type& state, const external_type* from, const external_type* from_end, std::size_t max) const;
     virtual int do_max_length() const noexcept;
     virtual int do_encoding() const noexcept;
     virtual bool do_always_noconv() const noexcept;
 }; // class codecvt

} // namespace std


namespace cynny {
namespace cynnypp {
namespace utilities {

/**
 * @brief atlas_locale represent the locale to be used by input streams in atlas
 */
extern const std::locale utilities_locale;


} // namespace utils
} // namespace atlas
}

#endif //SMUGGLER_LOCALES_H

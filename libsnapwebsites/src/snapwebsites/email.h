// Snap Websites Servers -- create a feed where you can write an email
// Copyright (c) 2016-2018  Made to Order Software Corp.  All Rights Reserved
//
// https://snapwebsites.org/
// contact@m2osw.com
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//
#pragma once

// snapwebsites lib
//
#include "qcaseinsensitivestring.h"
#include "snap_exception.h"

// qt serialization lib
//
#include <QtSerialization/QSerializationFieldTag.h>
#include <QtSerialization/QSerializationWriter.h>

// C++ lib
//
#include <map>
#include <memory>


namespace snap
{

class email_exception : public snap_exception
{
public:
    // no sub-name
    explicit email_exception(char const *        what_msg) : snap_exception("email", what_msg) {}
    explicit email_exception(std::string const & what_msg) : snap_exception("email", what_msg) {}
    explicit email_exception(QString const &     what_msg) : snap_exception("email", what_msg) {}
};


class email_exception_called_multiple_times : public email_exception
{
public:
    // no sub-name
    explicit email_exception_called_multiple_times(char const *        what_msg) : email_exception(what_msg) {}
    explicit email_exception_called_multiple_times(std::string const & what_msg) : email_exception(what_msg) {}
    explicit email_exception_called_multiple_times(QString const &     what_msg) : email_exception(what_msg) {}
};


class email_exception_called_after_end_header : public email_exception
{
public:
    // no sub-name
    explicit email_exception_called_after_end_header(char const *        what_msg) : email_exception(what_msg) {}
    explicit email_exception_called_after_end_header(std::string const & what_msg) : email_exception(what_msg) {}
    explicit email_exception_called_after_end_header(QString const &     what_msg) : email_exception(what_msg) {}
};


class email_exception_too_many_levels : public email_exception
{
public:
    // no sub-name
    explicit email_exception_too_many_levels(char const *        what_msg) : email_exception(what_msg) {}
    explicit email_exception_too_many_levels(std::string const & what_msg) : email_exception(what_msg) {}
    explicit email_exception_too_many_levels(QString const &     what_msg) : email_exception(what_msg) {}
};







// create email then use the sendemail() function to send it.
// if you want to save it and send it later, you can serialize/unserialize it too
//
class email
    : public QtSerialization::QSerializationObject
{
public:
    static int const        EMAIL_MAJOR_VERSION = 1;
    static int const        EMAIL_MINOR_VERSION = 0;

    typedef std::map<QCaseInsensitiveString, QString>   header_map_t;
    typedef std::map<QString, QString>                  parameter_map_t;

    enum class priority_t
    {
        EMAIL_PRIORITY_BULK = 1,
        EMAIL_PRIORITY_LOW,
        EMAIL_PRIORITY_NORMAL,
        EMAIL_PRIORITY_HIGH,
        EMAIL_PRIORITY_URGENT
    };

    class attachment
        : public QtSerialization::QSerializationObject
    {
    public:
        typedef std::vector<attachment>         vector_t;

                                attachment();
        virtual                 ~attachment();

        // data ("matter" of this attachment)
        //
        void                    set_data(QByteArray const & data, QString mime_type = QString());
        QByteArray              get_data() const;

        // header for this header
        //
        void                    set_content_disposition(QString const & filename, int64_t modification_date = 0, QString const & attachment_type = "attachment");
        void                    add_header(QString const & name, QString const & value);
        void                    remove_header(QString const & name);
        bool                    has_header(QString const & name) const;
        QString                 get_header(QString const & name) const;
        header_map_t const &    get_all_headers() const;

        // sub-attachment (one level available only)
        //
        void                    add_related(attachment const & data);
        int                     get_related_count() const;
        attachment &            get_related(int index) const;

        // "internal" functions used to save the data serialized
        //
        void                    unserialize(QtSerialization::QReader & r);
        virtual void            readTag(QString const & name, QtSerialization::QReader & r);
        void                    serialize(QtSerialization::QWriter & w, bool is_sub_attachment) const;

        bool                    operator == (attachment const & rhs) const;

    private:
        header_map_t            f_headers;
        QByteArray              f_data;
        bool                    f_is_sub_attachment = false;
        vector_t                f_sub_attachments; // for HTML data (images, css, ...)
    };

                            email();
    virtual                 ~email();

    // basic flags and strings
    //
    void                    set_branding(bool branding = true);
    bool                    get_branding() const;
    void                    set_cumulative(QString const & object);
    QString const &         get_cumulative() const;
    void                    set_site_key(QString const & site_key);
    QString const &         get_site_key() const;
    void                    set_email_path(QString const & email_path);
    QString const &         get_email_path() const;
    void                    set_email_key(QString const & site_key);
    QString const &         get_email_key() const;
    time_t                  get_time() const;

    // headers
    //
    void                    set_from(QString const & from);
    void                    set_to(QString const & to);
    void                    set_priority(priority_t priority = priority_t::EMAIL_PRIORITY_NORMAL);
    void                    set_subject(QString const & subject);
    void                    add_header(QString const & name, QString const & value);
    void                    remove_header(QString const & name);
    bool                    has_header(QString const & name) const;
    QString                 get_header(QString const & name) const;
    header_map_t const &    get_all_headers() const;

    // attachments
    //
    void                    set_body_attachment(attachment const & data);
    void                    add_attachment(attachment const & data);
    int                     get_attachment_count() const;
    attachment &            get_attachment(int index) const;

    // parameters (like headers but not included in email and names are
    // case sensitive)
    //
    void                    add_parameter(QString const & name, QString const & value);
    QString                 get_parameter(QString const & name) const;
    parameter_map_t const & get_all_parameters() const;

    // functions used to save the data serialized (used by the sendmail plugin)
    //
    void                    unserialize(QString const & data);
    virtual void            readTag(QString const & name, QtSerialization::QReader & r);
    QString                 serialize() const;

    bool                    send() const;

    bool                    operator == (email const & rhs) const;

private:
    bool                    f_branding = true;
    QString                 f_cumulative;
    QString                 f_site_key;
    QString                 f_email_path;
    QString                 f_email_key; // set on post_email()
    time_t                  f_time = static_cast<time_t>(-1);
    header_map_t            f_headers;
    attachment::vector_t    f_attachments;
    parameter_map_t         f_parameters;
};




} // namespace snap
// vim: ts=4 sw=4 et

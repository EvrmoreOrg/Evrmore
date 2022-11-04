// Copyright (c) 2011-2014 The Bitcoin Core developers
// Copyright (c) 2017-2019 The Raven Core developers
// Copyright (c) 2022 The Evrmore Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef EVRMORE_QT_EVRMOREADDRESSVALIDATOR_H
#define EVRMORE_QT_EVRMOREADDRESSVALIDATOR_H

#include <QValidator>

/** Base58 entry widget validator, checks for valid characters and
 * removes some whitespace.
 */
class EvrmoreAddressEntryValidator : public QValidator
{
    Q_OBJECT

public:
    explicit EvrmoreAddressEntryValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

/** Evrmore address widget validator, checks for a valid Evrmore address.
 */
class EvrmoreAddressCheckValidator : public QValidator
{
    Q_OBJECT

public:
    explicit EvrmoreAddressCheckValidator(QObject *parent);

    State validate(QString &input, int &pos) const;
};

#endif // EVRMORE_QT_EVRMOREADDRESSVALIDATOR_H

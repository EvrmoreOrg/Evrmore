// Copyright (c) 2022 The Evrmore Core developers

#ifndef EVRMORE_QT_TEST_WALLETTESTS_H
#define EVRMORE_QT_TEST_WALLETTESTS_H

#include <QObject>
#include <QTest>

class WalletTests : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void walletTests();
};

#endif // EVRMORE_QT_TEST_WALLETTESTS_H

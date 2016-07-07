//#include <QMessageBox>
#include <QDebug>
#include <QCoreApplication>

#include "ActiveMQModulePlugin.h"
//#ifdef _DEBUG
//#include "vld.h"
//#endif

ActiveMQModulePlugin::ActiveMQModulePlugin()
    : Interface()
    , m_currentPluginLoad(NULL)
{
    qRegisterMetaType<MQState>("MQState");
    m_pluginLoaderMap.clear();
    m_mqObjectMap.clear();
    m_outData.clear();
}

ActiveMQModulePlugin::~ActiveMQModulePlugin()
{
    qDebug()<<">> TEST INFO:"<<__FUNCTION__<<__LINE__;
//    foreach(QObject* object, m_mqObjectMap.values())
//    {
//        endConsumer(object->objectName());
//        object->deleteLater();
//    }
    foreach(QPluginLoader* pluginLoader, m_pluginLoaderMap.values())
    {
        qDebug()<<">> TEST INFO:"<<__FUNCTION__<<__LINE__ << pluginLoader->fileName();
        pluginLoader->unload();
        qDebug()<<">> TEST INFO:"<<__FUNCTION__<<__LINE__ << "end unload";
        delete pluginLoader;
        pluginLoader = NULL;
    }
    m_mqObjectMap.clear();
    m_pluginLoaderMap.clear();
    m_outData.clear();
    qDebug()<<">> TEST INFO:"<<__FUNCTION__<<__LINE__;
}

/*
 * ���ܣ���ʱ���ýӿ�
*/
QVariantMap ActiveMQModulePlugin::callFunc(QVariantMap& aMap)
{
    //QMessageBox::information(0,"info","callFunc");
//    qDebug() << __FUNCTION__ << __LINE__ << aMap;
    m_outData.clear();
    qDebug() << __FUNCTION__ << __LINE__;
    bool ok = true;
    QString funcName = aMap.value("FuncName").toString();
    int argCounts = aMap.value("ArgCounts").toInt(&ok);
    if (!ok)
    {
        m_outData["error"] = /*QStringLiteral*/("������������, ����������.");
        return m_outData;
    }
//    QList<QString> argList;
//    for(int i = 1; i <= argCounts; ++i)
//    {
//        QString argKey("Arg%1");
//        argList.append(aMap.value(argKey.arg(i)).toString());
//    }
    if (1 == argCounts)
    {
        QMetaObject::invokeMethod(this, funcName.toLatin1().data(), Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariantMap , m_outData),
                                  Q_ARG(QString, aMap.value("Arg1").toString()));

    }
    else if (2 == argCounts)
    {
        qDebug() << __FUNCTION__ << __LINE__;
        QMetaObject::invokeMethod(this, funcName.toLatin1().data(), Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariantMap , m_outData),
                                  Q_ARG(QString, aMap.value("Arg1").toString()),
                                  Q_ARG(QString, aMap.value("Arg2").toString()));

    }
    else if (3 == argCounts)
    {
        QMetaObject::invokeMethod(this, funcName.toLatin1().data(), Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariantMap , m_outData),
                                  Q_ARG(QString, aMap.value("Arg1").toString()),
                                  Q_ARG(QString, aMap.value("Arg2").toString()),
                                  Q_ARG(QString, aMap.value("Arg3").toString()));
    }
    else
    {
        m_outData["error"] = /*QStringLiteral*/("���޶�Ӧ�ķ���");
    }
    return m_outData;
}

/*
 * ���ܣ�ͨ����������ṩʵ����
 * ������
 *      aPluginName���������, ��ʽ���������ȥ��Plugin����(���磺DevReaderPlugin.dll���������DevReader)
*/
QObject* ActiveMQModulePlugin::createObject(const QString& aPluginName)
{
    setPluginName(aPluginName);
    Interface* mqObject = qobject_cast<Interface*>(m_pluginLoaderMap.value(aPluginName)->instance());
    m_mqObjectMap.insert(aPluginName, mqObject);
    return mqObject;

}

/*
 *����consumer
 * ��Σ�
 *      aPluginName: ����MQ�Ĳ������
 *      aBorkerURL��������url
 *      aChannelName�� ͨ������
 *      aUserName���û�����
 *      aPassword���û�����
 *      aClientId: ����ID
 *      aSubName : ��������
 * ����ֵ��
 *      retMap["code"] == 0 �ɹ� ����ʧ��
 *
*/
QVariantMap ActiveMQModulePlugin::startConsumer(const QString& aPluginName, const QString& aClientId,
                                                const QString &aSubName)
{
    qDebug() << __FUNCTION__ << __LINE__ << aPluginName;
    //��ȡMQ�������
    setPluginName(aPluginName);

    qDebug() <<  "NoticeMQPlugin->isLoaded(): " << m_currentPluginLoad->isLoaded();
    qDebug() <<  "m_currentMQPlugin: " << m_currentMQPlugin;

    QVariantMap mqFuncArgMap;
    mqFuncArgMap["FuncName"] = "startConsumer";
    mqFuncArgMap["ArgCounts"] = 2;
    mqFuncArgMap["Arg1"] = aClientId;
    mqFuncArgMap["Arg2"] = aSubName;

    QVariantMap retMap = m_currentMQPlugin->callFunc(mqFuncArgMap);
    return retMap;
}

QVariantMap ActiveMQModulePlugin::endConsumer(const QString& aPluginName)
{
    QVariantMap mqFuncArgMap;
    mqFuncArgMap["FuncName"] = "endConsumer";
    mqFuncArgMap["ArgCounts"] = 0;
    Interface * tempPlugin = qobject_cast<Interface*>(m_mqObjectMap.value(aPluginName));
    QVariantMap retMap = tempPlugin->callFunc(mqFuncArgMap);

    qDebug()<<">> TEST INFO:"<<__FUNCTION__<<__LINE__;
//    if (0 == retMap["code"] )
//    {
//        //ɾ��QPluginLoaderָ��
//        QPluginLoader* pluginLoader = m_pluginLoaderMap.value(aPluginName);
//        m_pluginLoaderMap.remove(aPluginName);
//        pluginLoader->unload();
//        delete pluginLoader;
//        pluginLoader = NULL;

////       //ɾ��MQ����ָ��
//       m_mqObjectMap.remove(aPluginName);
////       delete interface;
//       tempPlugin = NULL;

//    }
    qDebug()<<">> TEST INFO:"<<__FUNCTION__<<__LINE__;
    return retMap;
}

QVariantMap ActiveMQModulePlugin::sendMsgToServer(const QString& aPluginName, const QString& aMessage)
{
    qDebug() << __FUNCTION__ << __LINE__;
    setPluginName(aPluginName);

    QVariantMap mqFuncArgMap;
    mqFuncArgMap["FuncName"] = "sendMsgToServer";
    mqFuncArgMap["ArgCounts"] = 1;
    mqFuncArgMap["Arg1"] = aMessage;

    Interface * tempPlugin = qobject_cast<Interface*>(m_currentMQPlugin);

    QVariantMap retMap = tempPlugin->callFunc(mqFuncArgMap);
    return retMap;
}

/*
 * ���ܣ����ü���ʹ�õĲ�����ơ�
 * ������
 *      aPluginName���������, ��ʽ���������ȥ��Plugin����(���磺DevReaderPlugin.dll���������DevReader)
*/
void ActiveMQModulePlugin::setPluginName(const QString& aPluginName)
{
    qDebug() << __FUNCTION__ << __LINE__ << m_pluginLoaderMap;
    if(m_pluginLoaderMap.keys().contains(aPluginName))
    {
        m_currentPluginLoad = m_pluginLoaderMap.value(aPluginName);
    }
    else
    {
        QString pluginPath = QCoreApplication::applicationDirPath() + "/Plugin/" +  aPluginName;
        QString pluginName = pluginPath + "/" + aPluginName + "Plugin.dll";
        qDebug() << "NoticeMQPlugin Path: "<<  pluginName;
        m_currentPluginLoad = new QPluginLoader(pluginName);
        m_pluginLoaderMap.insert(aPluginName, m_currentPluginLoad);
    }
    m_currentMQPlugin = qobject_cast<Interface*>(m_currentPluginLoad->instance());
//    m_currentMQPlugin->moveToThread(&m_QThread);
    if (!m_mqObjectMap.contains(aPluginName))
    {
    m_mqObjectMap.insert(aPluginName, m_currentMQPlugin);
    }
    disconnect(m_currentMQPlugin, SIGNAL(MQConsumerMsgReceived(const QString&, const QString&)),
            this, SLOT(reveivedConMessage(const QString&, const QString&)));
    disconnect(m_currentMQPlugin, SIGNAL(MQProducterMsgReceived(const QString&, const QString&)),
            this, SLOT(reveivedProMessage(const QString&, const QString&)));
    disconnect(m_currentMQPlugin, SIGNAL(MQStateChanged(const QString&, MQState)),
            this, SIGNAL(MQStateChanged(const QString&, MQState)));

    connect(m_currentMQPlugin, SIGNAL(MQConsumerMsgReceived(const QString&, const QString&)),
            this, SLOT(reveivedConMessage(const QString&, const QString&)));
    connect(m_currentMQPlugin, SIGNAL(MQProducterMsgReceived(const QString&, const QString&)),
            this, SLOT(reveivedProMessage(const QString&, const QString&)));

    connect(m_currentMQPlugin, SIGNAL(MQStateChanged(const QString&, MQState)),
            this, SIGNAL(MQStateChanged(const QString&, MQState)));

}

//���ܵ����������ߵ���Ϣ
void ActiveMQModulePlugin::reveivedConMessage(const QString& aCurrentMQName, const QString& aMessage)
{
    qDebug() << __FUNCTION__ <<__LINE__ << "Reveived Message : " << aMessage;
    emit MQConsumerMsgReceived(aCurrentMQName, aMessage);
}

//���ܵ����������ߵ���Ϣ
void ActiveMQModulePlugin::reveivedProMessage(const QString& aCurrentMQName, const QString& aMessage)
{
    qDebug() << __FUNCTION__ <<__LINE__ << "Reveived Message : " << aMessage;
    emit MQProducterMsgReceived(aCurrentMQName, aMessage);
}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(ActiveMQModulePlugin, ActiveMQModulePlugin);
#endif // QT_VERSION < 0x050000

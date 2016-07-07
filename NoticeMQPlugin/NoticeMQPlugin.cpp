#include "NoticeMQPlugin.h"
#include <QDateTime>
#include <QDebug>
#include <QSettings>
#include <QCoreApplication>
//#ifdef _DEBUG
//#include "vld.h"
//#endif


NoticeMQPlugin::NoticeMQPlugin(QObject *parent)
    : Interface()
    , m_consumer(NULL)
    , m_producter(NULL)
    , m_consumerThread(NULL)
    , m_useTopic(false)
    , m_sessionTransacted(false)
    , m_isTopicPersistent(false)
    , m_brokerURI()
    , m_topicname()
    , m_username()
    , m_password()

{
    activemq::library::ActiveMQCPP::initializeLibrary();
    //Ϊ��������ܻ�ȡ���ĸ�MQ��������Ϣ��������objectName
    setObjectName("NoticeMQ");
    LoadConfig();
//    connect(this, SIGNAL(MqLibClosed()),
//            this, SLOT(closeMQLib()), Qt::QueuedConnection);

    Q_UNUSED(parent);
}
NoticeMQPlugin::~NoticeMQPlugin()
{
    if (NULL != m_consumer)
    {
    try {
            if(NULL == m_consumerThread)
        {
                return ;
            }
            m_consumer->close();
            delete m_consumer;
            m_consumer = NULL;
//            m_consumerThread->join();
//            delete m_consumerThread;
//            m_consumerThread = NULL;

        } catch (CMSException& e) {
            qDebug() << __FUNCTION__ << __LINE__;
            e.printStackTrace();
    }
    }
//    endConsumer();
    activemq::library::ActiveMQCPP::shutdownLibrary();
    qDebug()<<">> TEST INFO:"<<__FUNCTION__<<__LINE__;
}

void NoticeMQPlugin::LoadConfig()\
{
    QString pluginPath = QCoreApplication::applicationDirPath() + "/Plugin/" +  objectName();
    QString pluginName = pluginPath + "/" + objectName() + "Plugin.ini";
    qDebug()<<">> TEST INFO:"<<__FUNCTION__<<__LINE__ << pluginName;

    QSettings settings(pluginName,QSettings::IniFormat);
    m_brokerURI = settings.value("brokerURI","failover:(tcp://localhost:61616/)").toString().toStdString();
    m_useTopic = settings.value("useTopics",false).toBool();
    m_sessionTransacted = settings.value("sessionTransacted",false).toBool();
    m_isTopicPersistent = settings.value("persistent",false).toBool();
    m_topicname = settings.value("topicname","topic://NoticeTopic").toString().toStdString();
    m_queuename = settings.value("queuename","queue://NoticeQueue").toString().toStdString();
    m_username = settings.value("username","").toString().toStdString();
    m_password = settings.value("password","").toString().toStdString();

}


/*
 * QVariantMap��������
 * ��θ�ʽ��
 * FuncName��������
 * ArgCounts����������
 * Arg1����һ������
 * Arg2���ڶ�������
 * ��
 * ��
 * ��
 * �Դ�����(Ŀǰ��9�������ĺ��������˴���)
 * *************************
 * ���θ�ʽ
 * code : ����ֵ
 * data : ����Ϣ
 * error: ������Ϣ
*/
QVariantMap NoticeMQPlugin::callFunc(QVariantMap &aMap)
{
    m_outData.clear();
    qDebug() << __FUNCTION__ << __LINE__ ;
    bool ok = true;
    QString funcName = aMap.value("FuncName").toString();
    int argCounts = aMap.value("ArgCounts").toInt(&ok);
    if (!ok)
    {
        m_outData["error"] = /*QStringLiteral*/("������������, ����������.");
        return m_outData;
    }
    QList<QString> argList;
    for(int i = 1; i <= argCounts; ++i)
    {
        QString argKey("Arg%1");
        argList.append(aMap.value(argKey.arg(i)).toString());
    }
    if (0 == argCounts)
    {
        QMetaObject::invokeMethod(this, funcName.toLatin1().data(), Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariantMap , m_outData));

    }
    else if (1 == argCounts)
    {
        QMetaObject::invokeMethod(this, funcName.toLatin1().data(), Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariantMap , m_outData),
                                  Q_ARG(QString, aMap.value("Arg1").toString()));

    }

    else if (2 == argCounts)
    {
        QMetaObject::invokeMethod(this, funcName.toLatin1().data(), Qt::DirectConnection,
                                  Q_RETURN_ARG(QVariantMap , m_outData),
                                  Q_ARG(QString, aMap.value("Arg1").toString()),
                                  Q_ARG(QString, aMap.value("Arg2").toString()));
    }
    else
    {
        m_outData["error"] = /*QStringLiteral*/("���޶�Ӧ�ķ���");
    }
    return m_outData;
}

/*
 *����consumer
 * ��Σ�
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
QVariantMap NoticeMQPlugin::startConsumer(const QString& aClientId, const QString& aSubName)
{
    QVariantMap retMap;
    std::string clientId = aClientId.toStdString();
    std::string subName = aSubName.toStdString();


    m_consumer = new Consumer(m_topicname,m_username,m_password, m_brokerURI,
                              clientId, subName,m_isTopicPersistent,
                              m_useTopic, m_sessionTransacted);
    disconnect(m_consumer, SIGNAL(messageReceived(const QString&)),
                this, SLOT(reveivedConMessage(const QString& )));
    disconnect(m_consumer, SIGNAL(MQStateChanged(MQState)),
               this, SLOT(changeMQState(MQState)));

    connect(m_consumer, SIGNAL(messageReceived(const QString&)),
            this, SLOT(reveivedConMessage(const QString& )));
    connect(m_consumer, SIGNAL(MQStateChanged(MQState)),
            this, SLOT(changeMQState(MQState)));
    qDebug() << __FUNCTION__ << __LINE__ << "m_consumerThread == NULL";
    m_consumerThread = new Thread(m_consumer);
    m_consumerThread->start();
    qDebug() << __FUNCTION__ << __LINE__ << QStringLiteral("emit MQStateChanged�ź�");
    retMap["code"] = 0;
    return retMap;
}

QVariantMap NoticeMQPlugin::endConsumer()
{
    QVariantMap retMap;
    if (NULL != m_consumer)
    {
    try{
        qDebug() << __FUNCTION__ << __LINE__;
        if(NULL == m_consumerThread)
        {
            retMap["code"] = -1;
            return retMap;
        }
            m_consumer->close();
            delete m_consumer;
            m_consumer = NULL;

//            m_consumerThread->join();
            qDebug() << __FUNCTION__ << __LINE__  << "Thread isAlive:"<< m_consumerThread->isAlive();
        delete m_consumerThread;
        m_consumerThread = NULL;

            qDebug() << __FUNCTION__ << __LINE__  << "end interrupt Thread";

    } catch (CMSException& e) {
        qDebug() << __FUNCTION__ << __LINE__;
        e.printStackTrace();
    }
    }
    retMap["code"] = 0;
    return retMap;
}

QVariantMap NoticeMQPlugin::sendMsgToServer(const QString& aMessage)
{
    QVariantMap retMap;
    //Test SendMsg Code begin
    qDebug()<<">> TEST INFO:"<<__FUNCTION__<<__LINE__;
    if(NULL == m_producter)
    {
        m_producter = new Producter(m_queuename, m_username, m_password, m_brokerURI);
        connect(m_producter, SIGNAL(messageReceived(const QString&)),
                this, SLOT(reveivedProMessage(const QString& )),Qt::QueuedConnection);
    }
//    m_QThread = new QThread();
//    m_producter->moveToThread(m_QThread);
    m_producter->sendMsg(aMessage);

    //Test SendMsg Code end
    retMap["code"] = 0;
    return retMap;
}

//���ܵ����������ߵ���Ϣ
void NoticeMQPlugin::reveivedConMessage(const QString& aMessage)
{
    qDebug() << __FUNCTION__ <<__LINE__ << "Reveived Message : " << aMessage;
    emit MQConsumerMsgReceived(objectName(), aMessage);
}

//���ܵ����������ߵ���Ϣ
void NoticeMQPlugin::reveivedProMessage(const QString& aMessage)
{
    qDebug() << __FUNCTION__ <<__LINE__ << "Reveived Message : " << aMessage;
    emit MQProducterMsgReceived(objectName(), aMessage);
    if(NULL != m_producter)
    {
        disconnect(m_producter, SIGNAL(messageReceived(const QString&)),
                    this, SLOT(reveivedProMessage(const QString& )));
        delete m_producter;
        m_producter = NULL;
    }
}

void NoticeMQPlugin::changeMQState(MQState aState)
{
    if (aState == MQCLOSE)
    {
        m_consumerThread->join();
    }
    emit MQStateChanged(objectName(), aState);
}

////����Thread�˳���Ҫʱ�䣬���ͨ���ź���ʱ�ر�MQ
//void NoticeMQPlugin::closeMQLib()
//{
//    qDebug() << __FUNCTION__ <<__LINE__ ;
//    delete m_consumerThread;
//    m_consumerThread = NULL;
//}

#if QT_VERSION < 0x050000
Q_EXPORT_PLUGIN2(NoticeMQPlugin, NoticeMQPlugin);
#endif // QT_VERSION < 0x050000


